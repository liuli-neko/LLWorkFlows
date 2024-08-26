#pragma once

#include <condition_variable>
#include <functional>
#include <mutex>

#include "thread.hpp"
#include "threadworker.hpp"

LLWFLOWS_NS_BEGIN

struct TaskDescription {
    int                                       specifyWorkerId = -1;
    std::vector<std::shared_ptr<TaskPromise>> dependencies    = {};
};
class ThreadPool {
public:
    ThreadPool(size_t numThreads);
    ~ThreadPool();
    std::shared_ptr<TaskPromise> addTask(std::function<void()> task, const TaskDescription& desc = TaskDescription());
    int                          cancel(std::shared_ptr<TaskPromise> task);
    void                         wait(std::shared_ptr<TaskPromise> task);
    void                         start();
    void                         stop();
    void                         stopAndwaitAll();

private:
    std::shared_ptr<TaskPromise> retryTask(std::function<void()> task, std::shared_ptr<TaskPromise> promise,
                                           const TaskDescription& desc);

private:
#if LLWFLOWS_CPP_PLUS < 20
    std::mutex              mMutex;
    std::condition_variable mCondition;
#endif
    std::atomic<int>          mCurrentWorkerId{0};
    std::vector<ThreadWorker> mWorkers;
};
LLWFLOWS_NS_END