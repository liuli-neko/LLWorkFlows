#include "threadworker.hpp"

#include "log.hpp"

LLWFLOWS_NS_BEGIN
auto TaskPromise::state() const -> TaskState { return mState.load(std::memory_order_release); }

auto TaskPromise::workerId() const -> int { return mWorkerId; }

auto TaskPromise::workerIds() const -> const std::vector<int>& { return mWorkerIds; }

auto TaskPromise::cancel() -> int {
    auto taskState = mState.load(std::memory_order_release);
    do {
        // If the task is already completed or running or other, it cannot be cancel.
        if (taskState != TaskState::Queuing) {
            return -1;
        }
    } while (!mState.compare_exchange_weak(taskState, TaskState::Cancelled, std::memory_order_release,
                                           std::memory_order_relaxed));
    return 0;
}

auto TaskPromise::mutableState() -> std::atomic<TaskState>& { return mState; }

auto TaskPromise::mutableWorkerId() -> int& { return mWorkerId; }

auto TaskPromise::mutableWorkerIds() -> std::vector<int>& { return mWorkerIds; }

auto TaskPromise::runFailed() -> int {
    auto taskState = mState.load(std::memory_order_release);
    do {
        // other states, change to failed doesn't make sense.
        if (taskState != TaskState::Running) {
            return -1;
        }
    } while (!mState.compare_exchange_weak(taskState, TaskState::Cancelled, std::memory_order_release,
                                           std::memory_order_relaxed));
    return 0;
}

auto TaskPromise::resetState() -> int {
    auto taskState = mState.load(std::memory_order_release);
    do {
        // if state is queuing, it means the task is in initial state.
        // if state is running, its state can not reset.
        if (taskState == TaskState::Queuing || taskState == TaskState::Running) {
            return -1;
        }
    } while (!mState.compare_exchange_weak(taskState, TaskState::Queuing, std::memory_order_release,
                                           std::memory_order_relaxed));
    return 0;
}
auto TaskPromise::done() -> int {
    auto taskState = mState.load(std::memory_order_release);
    do {
        // other states, change to done doesn't make sense.
        if (taskState != TaskState::Running) {
            return -1;
        }
    } while (!mState.compare_exchange_weak(taskState, TaskState::Done, std::memory_order_release,
                                           std::memory_order_relaxed));

    return 0;
}
#if LLWFLOWS_CPP_PLUS >= 20
auto TaskPromise::wait() -> TaskState {
    while (mState.load(std::memory_order_release) == TaskState::Queuing ||
           mState.load(std::memory_order_release) == TaskState::Running) {
        mState.wait(TaskState::Queuing, std::memory_order_acquire);
        mState.wait(TaskState::Running, std::memory_order_acquire);
    }
    return mState.load(std::memory_order_release);
}
auto TaskPromise::notifyOne() -> void { mState.notify_one(); }
auto TaskPromise::notifyAll() -> void { mState.notify_all(); }
#endif

ThreadWorker::ThreadWorker(const int workerId, const int maxQueueSize) : mWorkerId(workerId), mTasks(maxQueueSize) {
    init(workerId);
}

auto ThreadWorker::init(const int workerId) -> int {
    if (workerId < 0) {
        return -1;
    }
    mWorkerId = workerId;
    setName((std::string("Worker-") + std::to_string(workerId)).c_str());
    mTasks.clear();
    return 0;
}

auto ThreadWorker::start() -> int {
    if (mWorkerId < 0) {
        LLWFLOWS_LOG_WARN("Worker id({}) is invalid, please init with valid id first.", mWorkerId);
        return -1;
    }
    if (isRunning()) {
        LLWFLOWS_LOG_WARN("Worker id({}) is already started.", mWorkerId);
        return -1;
    }
    mExit.store(false, std::memory_order_release);
    mExitAfterAllTasks.store(false, std::memory_order_release);
    Thread::start();
    return 0;
}

auto ThreadWorker::workerId() const -> int { return mWorkerId; }

auto ThreadWorker::post(std::function<void()> func) -> std::shared_ptr<TaskPromise> {
    return post([func = std::move(func)](const TaskPromise&) { func(); });
}

auto ThreadWorker::post(std::function<void()> func, std::shared_ptr<TaskPromise> taskPromise) -> int {
    return post([func = std::move(func)](const TaskPromise&) { func(); }, std::move(taskPromise));
}

auto ThreadWorker::post(std::function<void(TaskPromise&)> func) -> std::shared_ptr<TaskPromise> {
    Task task{std::move(func), std::make_shared<TaskPromise>()};
    task.taskPromise->mutableWorkerIds().push_back(mWorkerId);
    if (mTasks.push(task)) {
        return task.taskPromise;
    }
    if (mTasks.size() > 0 && mIsPaused) {
        mIsPaused = false;
        mConditionVar.notify_one();
    }
    return nullptr;
}

auto ThreadWorker::post(std::function<void(TaskPromise&)> func, std::shared_ptr<TaskPromise> taskPromise) -> int {
    Task task{std::move(func), std::move(taskPromise)};
    task.taskPromise->mutableWorkerIds().push_back(mWorkerId);
    if (mTasks.push(task)) {
        return 0;
    }
    if (mTasks.size() > 0 && mIsPaused) {
        mIsPaused = false;
        mConditionVar.notify_one();
    }
    return -1;
}

auto ThreadWorker::waitForExit() -> void {
    if (isJoinable() && (mExit || mExitAfterAllTasks)) {
        join();
    }
}

auto ThreadWorker::exit(bool AfterTaskInQueue) -> void {
    if (AfterTaskInQueue) {
        mExitAfterAllTasks.store(true, std::memory_order_release);
    } else {
        mExit.store(true, std::memory_order_release);
    }
    if (mIsPaused) {
        mIsPaused = false;
        mConditionVar.notify_one();
    }
}

auto ThreadWorker::taskQueue() -> SRingBuffer<Task>& { return mTasks; }

void ThreadWorker::run() {
    thread_local int count = 0;
    while (!mExit) {
        Task task;
        if (mTasks.pop(task)) {
            auto taskState = task.taskPromise->mutableState().load(std::memory_order_release);
            while (true) {
                if (taskState == TaskState::Queuing) {
                    if (task.taskPromise->mutableState().compare_exchange_weak(
                            taskState, TaskState::Running, std::memory_order_release, std::memory_order_relaxed)) {
                        task.taskPromise->mutableWorkerId() = mWorkerId;
                        task.func(*task.taskPromise);
                        task.taskPromise->done();
                        break;
                    }
                } else {
                    break;
                }
            }
        } else if (mTasks.size() == 0) {
            if (mExitAfterAllTasks) {
                mExit.store(true, std::memory_order_release);
                break;
            }
            count++;
            if (count > 0xffffff) {
                std::unique_lock<std::mutex> lock(mMutex);
                mIsPaused = true;
                while (mTasks.empty() && !mExit && !mExitAfterAllTasks && mIsPaused) {
                    mConditionVar.wait(
                        lock, [this]() { return mExit || !mTasks.empty() || !mIsPaused || mExitAfterAllTasks; });
                }
                count = 0;
            }
        }
    }
}

LLWFLOWS_NS_END
