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
        worker.exit();
        worker.waitForExit();
    }
}

std::shared_ptr<TaskPromise> ThreadPool::addTask(std::function<void()> task, const TaskDescription& desc) {
    if (mWorkers.empty()) {
        LLWFLOWS_LOG_WARN("No worker available");
        return std::shared_ptr<TaskPromise>();
    }
    auto promise = std::make_shared<TaskPromise>();
    return retryTask(task, promise, desc);
}

int  ThreadPool::cancel(std::shared_ptr<TaskPromise> task) { return task->cancel(); }

void ThreadPool::wait(std::shared_ptr<TaskPromise> task) {
#if LLWFLOWS_CPP_PLUS < 20
    std::guard<std::mutex> lock(mMutex);
    mCondition.wait(lock, [task]() {
        return (task->state() == TaskState::Queuing || task->state() == TaskState::Running) &&
               !mFinished.load(std::memory_order_release);
    });
#else
    task->wait();
#endif
}

void ThreadPool::start() {
    for (auto& worker : mWorkers) {
        worker.start();
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
    for (auto& worker : mWorkers) {
        worker.exit(true);
        worker.waitForExit();
    }
}

std::shared_ptr<TaskPromise> ThreadPool::retryTask(std::function<void()> task, std::shared_ptr<TaskPromise> promise,
                                                   const TaskDescription& desc) {
    if (mWorkers.empty()) {
        return std::shared_ptr<TaskPromise>();
    }
    promise->resetState();
    auto runner = [this, task, promise, desc](TaskPromise& _promise) {
        for (auto& deps : desc.dependencies) {
            if (deps->state() == TaskState::Cancelled) {
                return;
            }
            if (deps->state() != TaskState::Done) {
                _promise.runFailed();
                retryTask(task, promise, desc);
                return;
            }
        }
        task();
        _promise.done();
#if LLWFLOWS_CPP_PLUS < 20
        mCondition.notify_all();
#else
        promise->notifyAll();
#endif
    };
    if (desc.specifyWorkerId == -1) {
        if (mWorkers[mCurrentWorkerId++ % mWorkers.size()].post(runner, promise) == 0) {
            return promise;
        } else {
            LLWFLOWS_LOG_ERROR("Failed to post task to thread pool");
        }
    } else if (desc.specifyWorkerId > 0 && desc.specifyWorkerId < mWorkers.size()) {
        if (mWorkers[desc.specifyWorkerId].post(runner, promise) == 0) {
            return promise;
        }
    } else {
        LLWFLOWS_LOG_ERROR("Threadpool post invalid worker id: {}", desc.specifyWorkerId);
    }
    return std::shared_ptr<TaskPromise>();
}

LLWFLOWS_NS_END