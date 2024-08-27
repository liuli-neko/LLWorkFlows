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
    auto start() -> void;
    auto stop() -> void;
    // FIXME:
    // 任务在重试过程中有可能推到了其他线程，导致某些线程关闭，然后如果推到未执行的线程中，就会导致后续任务不再执行并退出。
    auto stopAndwaitAll() -> void;

protected:
    // after task failed by deps, will retry distributeTask
    virtual auto distributeTask(std::function<void()> task, const TaskDescription& desc)
        -> std::shared_ptr<TaskPromise>;
    virtual auto onWorkerIdle(const int workerId, const int IdleCount) -> void;
    auto makeTaskRetry(std::function<void()> task, const TaskDescription& desc)
        -> std::pair<std::function<void()>, TaskDescription*>;
    auto addTaskImp(std::function<void()> task, TaskDescription& desc, const int workerId)
        -> std::shared_ptr<TaskPromise>;
    /**
     * @brief next worker id by loop
     *
     * @return int -1 if no worker available
     */
    auto loopThroughWorkerId() -> int;
    auto workerIdSortByQueueSize(const int idx) -> int;
    auto workerIdSortByIdleLoop(const int idx) -> int;
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
};
LLWFLOWS_NS_END