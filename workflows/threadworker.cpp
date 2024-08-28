#include "threadworker.hpp"

#include "detail/log.hpp"

LLWFLOWS_NS_BEGIN
auto TaskPromise::state() const -> TaskState { return mState.load(std::memory_order_release); }

auto TaskPromise::workerId() const -> int { return mWorkerId.load(std::memory_order_release); }

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
#if LLWFLOWS_CPP_PLUS >= 20
    notifyAll();
#endif
    return 0;
}

auto TaskPromise::mutableState() -> std::atomic<TaskState>& { return mState; }

auto TaskPromise::mutableWorkerId() -> std::atomic<int>& { return mWorkerId; }

auto TaskPromise::mutableWorkerIds() -> std::vector<int>& { return mWorkerIds; }

auto TaskPromise::changeState(const TaskState old, const TaskState newState) -> int {
    auto taskState = mState.load(std::memory_order_release);
    do {
        // other states, change to failed doesn't make sense.
        if (taskState != old) {
            return -1;
        }
    } while (!mState.compare_exchange_weak(taskState, newState, std::memory_order_release, std::memory_order_relaxed));
#if LLWFLOWS_CPP_PLUS >= 20
    notifyAll();
#endif
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
#if LLWFLOWS_CPP_PLUS >= 20
    notifyAll();
#endif
    return 0;
}

auto TaskPromise::userData() -> void* { return mUserData; }

auto TaskPromise::userData(void* data) -> void { mUserData = data; }

auto TaskPromise::taskId() -> uint64_t { return mTaskId; }

auto TaskPromise::taskId(uint64_t id) -> void { mTaskId = id; }

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

ThreadWorker::ThreadWorker(const int workerId, const int maxQueueSize, const int maxIdleLoopCount)
    : mWorkerId(workerId), mTasks(maxQueueSize), mMaxIdleLoopCount(maxIdleLoopCount) {
    init(workerId);
}

auto ThreadWorker::init(const int workerId) -> int {
    if (workerId < 0) {
        return -1;
    }
    mWorkerId = workerId;
    setName((std::string("Worker-") + std::to_string(workerId)).c_str());
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
    auto promise = std::make_shared<TaskPromise>();
    promise->mutableWorkerIds().push_back(mWorkerId);
    if (mTasks.push({std::move(func), promise})) {
        mConditionVar.notify_one();
        return promise;
    }
    LLWFLOWS_LOG_WARN("Worker id({}) post task failed.", mWorkerId);
    return nullptr;
}

auto ThreadWorker::post(std::function<void()> func, std::shared_ptr<TaskPromise> taskPromise) -> int {
    taskPromise->mutableWorkerIds().push_back(mWorkerId);
    if (mTasks.push({std::move(func), taskPromise})) {
        mConditionVar.notify_one();
        return 0;
    }
    taskPromise->mutableWorkerIds().pop_back();
    LLWFLOWS_LOG_WARN("Worker id({}) post task failed. queue size: {}, capacity: {}", mWorkerId, mTasks.size(),
                      mTasks.capacity());
    return -1;
}

auto ThreadWorker::waitForExit() -> void {
    if (isJoinable() && (mExit.load(std::memory_order_release) || mExitAfterAllTasks.load(std::memory_order_release))) {
        join();
    }
}

auto ThreadWorker::exit(bool AfterTaskInQueue) -> void {
    if (AfterTaskInQueue) {
        mExitAfterAllTasks.store(true, std::memory_order_release);
    } else {
        mExit.store(true, std::memory_order_release);
    }
    if (mTasks.size() == 0) {
        mConditionVar.notify_one();
    }
}

auto ThreadWorker::taskQueue() -> SRingBuffer<Task>& { return mTasks; }

auto ThreadWorker::idleLoopCount() -> int { return mIdleLoopCount; }

auto ThreadWorker::maxIdleLoopCount() -> int { return mMaxIdleLoopCount; }

auto ThreadWorker::registerCallbackInIdleLoop(std::function<void(const int workId, const int count)> func) -> void {
    mCallbackInIdleLoop = func;
}

auto ThreadWorker::isIdle() -> bool {
    if (mIdleLoopCount.load(std::memory_order_release) >= mMaxIdleLoopCount) {
        return true;
    }
    return false;
}

void ThreadWorker::run() {
    while (!mExit) {
        Task task;
        if (mTasks.pop(task)) {
            mIdleLoopCount.store(0, std::memory_order_release);
            auto taskState = task.taskPromise->mutableState().load(std::memory_order_release);
            while (true) {
                if (taskState == TaskState::Queuing) {
                    if (task.taskPromise->mutableState().compare_exchange_weak(
                            taskState, TaskState::Running, std::memory_order_release, std::memory_order_relaxed)) {
                        task.taskPromise->mutableWorkerId() = mWorkerId;
                        task.func();
                        task.taskPromise->done();
                        break;
                    }
                } else {
                    break;
                }
            }
        } else if (mTasks.size() == 0) {
            mIdleLoopCount.fetch_add(1, std::memory_order_release);
            if (mCallbackInIdleLoop) {
                mCallbackInIdleLoop(mWorkerId, mIdleLoopCount.load(std::memory_order_release));
            }
            if (mExitAfterAllTasks && mTasks.size() == 0) {
                mExit.store(true, std::memory_order_release);
                break;
            }
            if (mIdleLoopCount.load(std::memory_order_release) > mMaxIdleLoopCount) {
                std::unique_lock<std::mutex> lock(mMutex);
                while (mTasks.empty() && !mExit && !mExitAfterAllTasks) {
                    mConditionVar.wait(lock, [this]() { return mExit || !mTasks.empty() || mExitAfterAllTasks; });
                }
                mIdleLoopCount.store(0, std::memory_order_release);
            }
        }
    }
    while (mTasks.size() > 0) {
        Task task;
        if (mTasks.pop(task)) {
            task.taskPromise->mutableWorkerId() = mWorkerId;
            task.taskPromise->cancel();
        }
    }
}

LLWFLOWS_NS_END
