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

enum class TaskState { Queuing = 0, Running, Done, Cancelled, Failed };

class TaskPromise {
public:
    TaskPromise() = default;
    auto state() const -> TaskState;
    auto workerId() const -> int;
    auto workerIds() const -> const std::vector<int>&;
    auto cancel() -> int;
    auto runFailed() -> int;
    auto resetState() -> int;
    auto done() -> int;
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
    auto mutableWorkerId() -> int&;
    auto mutableWorkerIds() -> std::vector<int>&;
    friend class ThreadWorker;

private:
    std::atomic<TaskState> mState{TaskState::Queuing};
    int                    mWorkerId{-1};
    std::vector<int>       mWorkerIds;
};

struct Task {
    std::function<void(TaskPromise&)> func;
    std::shared_ptr<TaskPromise>      taskPromise;
};

class LLWFLOWS_API ThreadWorker : Thread {
public:
    ThreadWorker(const int workerId = -1, const int maxQueueSize = 1024);
    ~ThreadWorker() override = default;

    auto init(const int workerId) -> int;
    auto start() -> int;
    auto workerId() const -> int;
    auto post(std::function<void(TaskPromise&)> func) -> std::shared_ptr<TaskPromise>;
    auto post(std::function<void(TaskPromise&)> func, std::shared_ptr<TaskPromise> taskPromise) -> int;
    auto post(std::function<void()> func) -> std::shared_ptr<TaskPromise>;
    auto post(std::function<void()> func, std::shared_ptr<TaskPromise> taskPromise) -> int;
    auto waitForExit() -> void;
    auto exit(bool AfterTaskInQueue = false) -> void;
    auto taskQueue() -> SRingBuffer<Task>&;

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
    int                     mWorkerId{-1};
    bool                    mIsPaused{false};
    std::atomic<bool>       mExit{false};
    std::atomic<bool>       mExitAfterAllTasks{false};
    SRingBuffer<Task>       mTasks;
    std::mutex              mMutex;
    std::condition_variable mConditionVar;
};

LLWFLOWS_NS_END