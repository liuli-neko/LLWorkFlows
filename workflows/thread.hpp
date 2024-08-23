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

public:
    Thread()                            = default;
    virtual ~Thread();
    Thread(Thread&&)                    = default;
    auto operator=(Thread&&) -> Thread& = default;

    auto setName(const char* name) -> void;
    auto name() const -> const char*;
    auto start(std::function<void()> func) -> void;
    auto start() -> void;
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

private:
    std::string                  mName{};
    std::function<void()>        mFunc{};
    std::thread::id              mId{};
    std::atomic<bool>            mIsRunning{false};
    std::unique_ptr<std::thread> mThreadMetaData{};
};

LLWFLOWS_NS_END  // namespace llworkflows