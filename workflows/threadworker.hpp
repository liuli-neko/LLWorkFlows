#pragma once

#include <atomic>
#include <condition_variable>
#include <functional>
#include <future>
#include <mutex>
#include <queue>

#include "thread.hpp"

LLWFLOWS_NS_BEGIN

class LLWFLOWS_API ThreadWorker : public Thread {
public:
    ThreadWorker()                               = default;
    ~ThreadWorker() override                     = default;
    ThreadWorker(const ThreadWorker&)            = delete;
    ThreadWorker(ThreadWorker&&)                 = delete;

    ThreadWorker& operator=(const ThreadWorker&) = delete;
    ThreadWorker& operator=(ThreadWorker&&)      = delete;

    void          init();
    void          invoke(std::function<void()> func);
    

protected:
    void run() override;

private:
    std::queue<std::function<void()>> mTasks;
    std::mutex                        mMutex;
    std::condition_variable           mCv;
    std::atomic<bool>                 mStop{false};
};

LLWFLOWS_NS_END