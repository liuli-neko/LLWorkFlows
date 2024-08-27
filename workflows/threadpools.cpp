#include "threadpools.hpp"

#include "log.hpp"

LLWFLOWS_NS_BEGIN

ThreadPool::ThreadPool(size_t numThreads) : mWorkers(numThreads) {
    for (size_t i = 0; i < numThreads; ++i) {
        mWorkers[i].init(i);
    }
}

ThreadPool::~ThreadPool() {
    for (auto& worker : mWorkers) {
        if (worker.isRunning()) {
            worker.exit();
            worker.waitForExit();
        }
    }
}

std::shared_ptr<TaskPromise> ThreadPool::addTask(std::function<void()> task, const TaskDescription& desc) {
    if (mWorkers.empty()) {
        LLWFLOWS_LOG_WARN("No worker available");
        return std::shared_ptr<TaskPromise>();
    }
    return distributeTask(task, desc);
}

int  ThreadPool::cancel(std::shared_ptr<TaskPromise> task) { return task->cancel(); }

void ThreadPool::wait(std::shared_ptr<TaskPromise> task) {
    while (task->state() == (TaskState)TaskStateCustom::TaskDependsUnfinish || task->state() == TaskState::Queuing ||
           task->state() == TaskState::Running) {
#if LLWFLOWS_CPP_PLUS < 20
        std::guard<std::mutex> lock(mMutex);
        while (task->state() == TaskState::Queuing || task->state() == TaskState::Running) {
            mCondition.wait(lock, [task]() {
                return (task->state() == TaskState::Queuing || task->state() == TaskState::Running);
            });
        }
#else
        task->wait();
#endif
    }
}

void ThreadPool::start() {
    for (auto& worker : mWorkers) {
        worker.start();
        worker.registerCallbackInIdleLoop(
            std::bind(&ThreadPool::onWorkerIdle, this, std::placeholders::_1, std::placeholders::_2));
    }
}

void ThreadPool::stop() {
    for (auto& worker : mWorkers) {
        worker.exit();
    }
    for (auto& worker : mWorkers) {
        worker.waitForExit();
    }
}

void ThreadPool::stopAndwaitAll() {
    // FIXME:
    // 任务在重试过程中有可能推到了其他线程，导致某些线程关闭，然后如果推到未执行的线程中，就会导致后续任务不再执行并退出。
    for (auto& worker : mWorkers) {
        worker.exit(true);
        worker.waitForExit();
    }
}

auto ThreadPool::distributeTask(std::function<void()> task, const TaskDescription& desc)
    -> std::shared_ptr<TaskPromise> {
    if (desc.specifyWorkerId != -1) {
        if (desc.specifyWorkerId >= mWorkers.size() || desc.specifyWorkerId < 0) {
            LLWFLOWS_LOG_ERROR("Invalid worker id: {}", desc.specifyWorkerId);
            return nullptr;
        }
        auto [taskWithRetry, descptr] = makeTaskRetry(std::move(task), desc);
        return addTaskImp(std::move(taskWithRetry), *descptr, desc.specifyWorkerId);
    }
    auto [taskWithRetry, descptr] = makeTaskRetry(std::move(task), desc);
    int workerId                  = -1;
    switch (desc.priority) {
        case TaskPriority::Low:
            workerId = workerIdSortByQueueSize(mWorkers.size() - 2);
            break;
        case TaskPriority::Normal:
            workerId = workerIdSortByQueueSize(0);
            break;
        case TaskPriority::High:
            workerId = workerIdSortByIdleLoop(0);
            if (workerId == -1) {
                workerId = workerIdSortByQueueSize(0);
            }
    }
    if (workerId == -1) {
        workerId = loopThroughWorkerId();
    }
    if (descptr->retryCount > 10) {
        for (auto& dep : descptr->dependencies) {
            if (dep->workerIds().size() > 0 && dep->state() != TaskState::Done) {
                workerId = dep->workerIds().back();
                break;
            }
        }
    }
    // LLWFLOWS_LOG_INFO("add task[{}] to worker {} with priority {}, workerqueuesize {}, idle count {}", descptr->name, workerId,
    //                   (int)desc.priority, mWorkers[workerId].taskQueue().size(), mWorkers[workerId].idleLoopCount());
    return addTaskImp(std::move(taskWithRetry), *descptr, workerId);
}

auto ThreadPool::onWorkerIdle(const int workerId, const int IdleCount) -> void {
    if (IdleCount >= mWorkers[workerId].maxIdleLoopCount() / 2) {
        auto idx = workerIdSortByQueueSize(mWorkers.size() - 1);
        if (idx != -1) {
            Task task;
            if (mWorkers[idx].taskQueue().size() > 1 && mWorkers[idx].taskQueue().pop(task)) {
                mWorkers[workerId].taskQueue().push(std::move(task));
            }
        }
    }
}

auto ThreadPool::makeTaskRetry(std::function<void()> task, const TaskDescription& desc)
    -> std::pair<std::function<void()>, TaskDescription*> {
    std::shared_ptr<TaskDescription> descptr =
        std::shared_ptr<TaskDescription>(new TaskDescription{std::move(desc)}, [this, task](TaskDescription* p) {
            if (p->promise == nullptr) {
                delete p;
                return;
            }
#if LLWFLOWS_CPP_PLUS < 20
            if (p->promise.state() != TaskState::Running || p->promise.state() != TaskState::Queuing) {
                mCondition.notify_all();
            }
#endif
            if (p->promise->state() == (TaskState)TaskStateCustom::TaskDependsUnfinish) {
                p->retryCount++;
                distributeTask(task, *p);
            }
            if (p->promise->state() == TaskState::Done) {
                // LLWFLOWS_LOG_INFO("task[{}] fished in worker {}, retry {}, priority {}.", p->name,
                //                   p->promise->workerId(), p->retryCount, (int)p->priority);
            }
            delete p;
        });
    return std::make_pair(
        [task, desc = descptr]() {
            for (auto& deps : desc->dependencies) {
                if (deps->state() != TaskState::Done) {
                    desc->promise->changeState(TaskState::Running, (TaskState)TaskStateCustom::TaskDependsUnfinish);
                    return;
                }
            }
            task();
        },
        descptr.get());
}

std::shared_ptr<TaskPromise> ThreadPool::addTaskImp(std::function<void()> task, TaskDescription& desc,
                                                    const int workerId) {
    if (desc.promise == nullptr) {
        desc.promise = std::make_shared<TaskPromise>();
    }
    desc.promise->resetState();
    for (auto& dep : desc.dependencies) {
        if (dep->state() == TaskState::Cancelled) {
            desc.promise->cancel();
            return desc.promise;
        }
    }
    if (workerId >= 0 && workerId < mWorkers.size()) {
        if (mWorkers[workerId].post(std::move(task), desc.promise) == 0) {
            return desc.promise;
        }
    } else {
        LLWFLOWS_LOG_ERROR("Threadpool post invalid worker id: {}", workerId);
    }
    return std::shared_ptr<TaskPromise>();
}

auto ThreadPool::loopThroughWorkerId() -> int { return mCurrentWorkerId++ % mWorkers.size(); }

auto ThreadPool::workerIdSortByQueueSize(const int idx) -> int {
    if (idx < 0 || idx >= mWorkers.size()) {
        return -1;
    }
    std::vector<std::pair<int, std::pair<int, int>>> workerQueueSize;
    for (int i = 0; i < mWorkers.size(); i++) {
        if (mWorkers[i].taskQueue().size() < mWorkers[idx].taskQueue().capacity()) {
            workerQueueSize.push_back(
                std::make_pair(i, std::make_pair(mWorkers[i].taskQueue().size(), mWorkers[i].idleLoopCount())));
        }
    }
    std::sort(workerQueueSize.begin(), workerQueueSize.end(), [](const auto& a, const auto& b) {
        if (a.second.first == b.second.first) {
            return a.second.second < b.second.second;
        }
        return a.second.first < b.second.first;
    });
    return workerQueueSize[idx].first;
}

auto ThreadPool::workerIdSortByIdleLoop(const int idx) -> int {
    if (idx < 0 || idx >= mWorkers.size()) {
        return -1;
    }
    std::vector<std::pair<int, int>> workerIdleLoop;
    for (int i = 0; i < mWorkers.size(); i++) {
        if (mWorkers[i].idleLoopCount() > 0) {
            workerIdleLoop.push_back(std::make_pair(i, mWorkers[i].idleLoopCount()));
        }
    }
    std::sort(workerIdleLoop.begin(), workerIdleLoop.end(),
              [](const auto& a, const auto& b) { return a.second < b.second; });
    if (workerIdleLoop.size() <= idx) {
        return -1;  // no specific idle loop
    }
    return workerIdleLoop[idx].first;
}

auto ThreadPool::workers() -> std::vector<ThreadWorker>& { return mWorkers; }

auto ThreadPool::workers() const -> const std::vector<ThreadWorker>& { return mWorkers; }

auto ThreadPool::workerCount() const -> int { return mWorkers.size(); }

LLWFLOWS_NS_END