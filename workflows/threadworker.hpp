#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <vector>

#include "sringbuffer.hpp"
#include "thread.hpp"

LLWFLOWS_NS_BEGIN

enum class TaskState { Queuing = 0, Running, Done, Cancelled, Custom = 0x8000 };

class TaskPromise {
public:
    TaskPromise() noexcept = default;
    virtual ~TaskPromise() = default;
    auto state() const -> TaskState;
    auto workerId() const -> int;
    auto workerIds() const -> const std::vector<int>&;
    auto cancel() -> int;
    auto changeState(const TaskState old, const TaskState newstate) -> int;
    auto resetState() -> int;
    auto done() -> int;
    auto userData() -> void*;
    auto userData(void* data) -> void;
    auto taskId() -> uint64_t;
    auto taskId(uint64_t id) -> void;
#if LLWFLOWS_CPP_PLUS >= 20
    auto wait() -> TaskState;
    auto notifyOne() -> void;
    auto notifyAll() -> void;
#endif
protected:
    /**
     * @brief state of the task
     *
     * @note this info while be accessed by multiple threads
     * so if you want to change it, make sure it is from the exact state transfer to another state.
     * while state not meet the requirement, stop the transfer. and don't continue, Go ahead and re-check the state.
     *
     * @return std::atomic<TaskState>&
     */
    auto mutableState() -> std::atomic<TaskState>&;
    auto mutableWorkerId() -> std::atomic<int>&;
    auto mutableWorkerIds() -> std::vector<int>&;
    friend class ThreadWorker;

private:
    TaskPromise(TaskPromise&&)                 = delete;
    TaskPromise(const TaskPromise&)            = delete;
    TaskPromise& operator=(TaskPromise&&)      = delete;
    TaskPromise& operator=(const TaskPromise&) = delete;

private:
    std::atomic<TaskState> mState{TaskState::Queuing};
    std::atomic<int>       mWorkerId{-1};
    std::vector<int>       mWorkerIds;
    uint64_t               mTaskId   = 0;
    void*                  mUserData = nullptr;
};

struct Task {
    inline Task() noexcept            = default;
    inline Task(const Task&) noexcept = default;
    inline Task(Task&&) noexcept      = default;
    inline Task(std::function<void()> func, std::shared_ptr<TaskPromise> taskPromise) noexcept
        : func(std::move(func)), taskPromise(std::move(taskPromise)) {}
    inline Task&                 operator=(const Task&) noexcept = default;
    inline Task&                 operator=(Task&&) noexcept      = default;
    std::function<void()>        func;
    std::shared_ptr<TaskPromise> taskPromise;
};

class LLWFLOWS_API ThreadWorker : Thread {
public:
    ThreadWorker(const int workerId = -1, const int maxQueueSize = 1024, const int maxIdleLoopCount = 0xffffff);
    ~ThreadWorker() override = default;

    auto init(const int workerId) -> int;
    auto start() -> int;
    auto workerId() const -> int;
    auto post(std::function<void()> func) -> std::shared_ptr<TaskPromise>;
    auto post(std::function<void()> func, std::shared_ptr<TaskPromise> taskPromise) -> int;
    auto waitForExit() -> void;
    auto exit(bool AfterTaskInQueue = false) -> void;
    auto taskQueue() -> SRingBuffer<Task>&;
    auto idleLoopCount() -> int;
    auto maxIdleLoopCount() -> int;
    auto registerCallbackInIdleLoop(std::function<void(const int workId, const int count)> func) -> void;
    auto isIdle() -> bool;

    using Thread::isRunning;
    using Thread::maxPriority;
    using Thread::name;
    using Thread::priority;
    using Thread::setPriority;

protected:
    void run() override;

private:
    ThreadWorker(const ThreadWorker&)                    = delete;
    ThreadWorker(ThreadWorker&&)                         = delete;

    auto operator=(const ThreadWorker&) -> ThreadWorker& = delete;
    auto operator=(ThreadWorker&&) -> ThreadWorker&      = delete;

private:
    int                                       mWorkerId{-1};
    std::atomic<bool>                         mExit{false};
    std::atomic<bool>                         mExitAfterAllTasks{false};
    SRingBuffer<Task>                         mTasks;
    std::mutex                                mMutex;
    std::condition_variable                   mConditionVar;
    std::atomic<int>                          mIdleLoopCount{0};
    const int                                 mMaxIdleLoopCount{0xffffff};
    std::function<void(const int, const int)> mCallbackInIdleLoop;
};

LLWFLOWS_NS_END