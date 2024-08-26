#pragma once

#include <atomic>
#include <functional>
#include <memory>
#include <string>
#include <thread>

#include "workflowsglobal.hpp"

LLWFLOWS_NS_BEGIN

class LLWFLOWS_API Thread {
public:
    static auto currentThread() -> Thread&;
    static auto makeMetaObjectForCurrentThread(const char* name) -> void;

    enum SchedPolicy { SchedRoundRobin = SCHED_RR, SchedFIFO = SCHED_FIFO, SchedOther = SCHED_OTHER };
public:
    Thread();
    virtual ~Thread();
    Thread(Thread&&)                    = default;
    auto operator=(Thread&&) -> Thread& = default;

    auto setName(const char* name) -> void;
    auto name() const -> const char*;
    auto start(std::function<void()> func) -> void;
    auto start() -> void;
    auto setPriority(const int policy, const int priority) -> void;
    auto priority() const -> std::pair<int, int>;
    auto maxPriority() const -> int;
    auto join() -> void;
    auto detach() -> void;
    auto id() const -> uint64_t;
    auto swap(Thread& other) -> void;

    auto isRunning() const -> bool;
    auto isDetached() const -> bool;
    auto isJoinable() const -> bool;

protected:
    virtual auto run() -> void;

private:
    Thread(const Thread&)                    = delete;
    auto operator=(const Thread&) -> Thread& = delete;
    auto runImpl() -> void;
    auto setPriorityImpl(const int policy, const int priority) -> void;

private:
    std::string                  mName{};
    std::function<void()>        mFunc{};
    std::thread::id              mId{};
    std::atomic<bool>            mIsRunning{false};
    std::unique_ptr<std::thread> mThreadMetaData{};
    int                          mPolicy{};
    int                          mPriority{};
};

LLWFLOWS_NS_END  // namespace llworkflows