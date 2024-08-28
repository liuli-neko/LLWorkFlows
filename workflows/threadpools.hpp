#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>

#include "thread.hpp"
#include "threadworker.hpp"

LLWFLOWS_NS_BEGIN

enum class TaskPriority { High, Normal, Low };

struct TaskDescription {
    std::string                               name            = {};
    int                                       specifyWorkerId = -1;
    std::vector<std::shared_ptr<TaskPromise>> dependencies    = {};
    std::shared_ptr<TaskPromise>              promise         = nullptr;
    TaskPriority                              priority        = TaskPriority::Normal;
    int                                       retryCount      = 0;
};
class ThreadPool {
    enum TaskStateCustom {
        TaskDependsUnfinish = (int)TaskState::Custom + 1,
    };

public:
    ThreadPool(size_t numThreads);
    virtual ~ThreadPool();
    auto addTask(std::function<void()> task, const TaskDescription& desc = TaskDescription())
        -> std::shared_ptr<TaskPromise>;
    auto cancel(std::shared_ptr<TaskPromise> task) -> int;
    auto wait(std::shared_ptr<TaskPromise> task) -> void;
    /**
     * @brief start workers in thread pool
     *
     * enableWorkStealing: enable work stealing
     *
     * @note
     * if enableWorkStealing is true, the thread pool will try to steal tasks from other threads when a thread is idle.
     * and you should not add task with specifyWorkerId, because all task may be stolen by other threads.
     *
     * @param enableWorkStealing
     */
    auto start(const bool enableWorkStealing = false) -> void;
    auto stop() -> void;
    // FIXME:
    // 任务在重试过程中有可能推到了其他线程，导致某些线程关闭，然后如果推到未执行的线程中，就会导致后续任务不再执行并退出。
    auto stopAndwaitAll() -> void;

protected:
    // after task failed by deps, will retry distributeTask
    virtual auto distributeTask(std::function<void()> task, const TaskDescription& desc)
        -> std::shared_ptr<TaskPromise>;
    virtual auto onWorkerIdle(const int workerId, const int IdleCount) -> void;
    ///> @brief pack task to support some properties like retry, deps, etc. and update description info
    auto packTask(std::function<void()> task, const TaskDescription& desc)
        -> std::pair<std::function<void()>, TaskDescription*>;
    auto addTaskImp(std::function<void()> task, TaskDescription& desc, const int workerId)
        -> std::shared_ptr<TaskPromise>;
    /**
     * @brief next worker id by loop
     *
     * @return int -1 if no worker available
     */
    auto pickWorkerIdByRoundRobin() -> int;
    ///> @brief The idx smaller the workload more little
    auto pickWorkerIdByWorkload(const int idx) -> int;
    ///> @brief The idx smaller the more idle
    auto pickWorkerIdByIdleness(const int idx) -> int;
    /// @brief The idx smaller the queue size more little
    auto pickWorkerIdByQueueSize(const int idx) -> int;
    auto pickWorkerIdByRandom() -> int;
    auto workers() -> std::vector<ThreadWorker>&;
    auto workers() const -> const std::vector<ThreadWorker>&;
    auto workerCount() const -> int;

private:
#if LLWFLOWS_CPP_PLUS < 20
    std::mutex              mMutex;
    std::condition_variable mCondition;
#endif
    std::atomic<int>          mCurrentWorkerId{0};
    std::vector<ThreadWorker> mWorkers;
    uint64_t                  mTaskCount{0};
};
LLWFLOWS_NS_END