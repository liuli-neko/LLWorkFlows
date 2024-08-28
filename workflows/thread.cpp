#include "thread.hpp"

#include <map>

#include "detail/log.hpp"

LLWFLOWS_NS_BEGIN

static std::map<std::thread::id, Thread*> kThreads;

// ------------ Thread ------------
auto Thread::setName(const char* name) -> void {
    mName = name;
    if (isRunning()) {
        pthread_t thread_handle = mThreadMetaData->native_handle();
        pthread_setname_np(thread_handle, name);
    }
}

auto Thread::name() const -> const char* { return mName.c_str(); }

auto Thread::start(std::function<void()> func) -> void {
    LLWFLOWS_ASSERT(!mThreadMetaData || !mThreadMetaData->joinable(), "Thread already running");
    mFunc = func;
    start();
}

auto Thread::start() -> void {
    LLWFLOWS_ASSERT(!mThreadMetaData || !mThreadMetaData->joinable(), "Thread already running");
    if (mThreadMetaData) {
        auto it = kThreads.find(mId);
        if (it != kThreads.end()) {
            kThreads.erase(it);
        }
    }
    mThreadMetaData.reset(new std::thread(std::bind(&Thread::runImpl, this)));
    mId           = mThreadMetaData->get_id();
    kThreads[mId] = this;
}

auto Thread::setPriority(const int policy, const int priority) -> void {
    mPriority = priority;
    mPolicy   = policy;
    setPriorityImpl(policy, priority);
}

auto Thread::priority() const -> std::pair<int, int> {
    pthread_attr_t attr;
    pthread_attr_init(&attr);
    int                policy;
    struct sched_param param;
    pthread_attr_getschedpolicy(&attr, &policy);
    if (mThreadMetaData) {
        pthread_t thread_handle = mThreadMetaData->native_handle();
        pthread_getschedparam(thread_handle, &policy, &param);
    } else {
        pthread_getschedparam(pthread_self(), &policy, &param);
    }
    return {policy, param.sched_priority};
}

auto Thread::maxPriority() const -> int { return sched_get_priority_max(mPolicy); }

auto Thread::join() -> void {
    LLWFLOWS_ASSERT(mThreadMetaData && mThreadMetaData->joinable(), "Thread not running");
    if (mThreadMetaData) mThreadMetaData->join();
}

auto Thread::detach() -> void {
    LLWFLOWS_ASSERT(mThreadMetaData && mThreadMetaData->joinable(), "Thread not running");
    if (mThreadMetaData) mThreadMetaData->detach();
}

auto Thread::id() const -> uint64_t {
    if (mThreadMetaData) {
        return std::hash<std::thread::id>()(mThreadMetaData->get_id());
    } else {
        return std::hash<std::thread::id>()(mId);
    }
}

auto Thread::swap(Thread& other) -> void {
    std::swap(mThreadMetaData, other.mThreadMetaData);
    std::swap(mFunc, other.mFunc);
    std::swap(mName, other.mName);
    std::swap(mId, other.mId);
    mIsRunning = other.mIsRunning.load();
}

auto Thread::isRunning() const -> bool {
    if (mIsRunning.load() || isJoinable()) {
        return true;
    }
    return false;
}

auto Thread::isDetached() const -> bool {
    if (mThreadMetaData && !mThreadMetaData->joinable() && mIsRunning.load()) {
        return true;
    }
    return false;
}

auto Thread::isJoinable() const -> bool {
    if (mThreadMetaData && mThreadMetaData->joinable()) {
        return true;
    }
    return false;
}

auto Thread::runImpl() -> void {
    mIsRunning = true;
    pthread_setname_np(pthread_self(), mName.c_str());
    setPriorityImpl(mPolicy, mPriority);
    run();
    mIsRunning = false;
}

auto Thread::setPriorityImpl(const int policy, const int priority) -> void {
    pthread_attr_t attr;
    if (auto ret = pthread_attr_init(&attr); ret != 0) {
        LLWFLOWS_LOG_ERROR("Failed to initialize thread({}) attribute, error: {}", name(), strerror(ret));
    }
    if (auto ret = pthread_attr_setschedpolicy(&attr, policy); ret != 0) {
        LLWFLOWS_LOG_ERROR("Failed to set thread({}) policy, error: {}", name(), strerror(ret));
    }
    struct sched_param param;
    param.sched_priority = priority;
    if (auto ret = pthread_attr_setschedparam(&attr, &param); ret != 0) {
        LLWFLOWS_LOG_ERROR("Failed to set thread param, error: {}", strerror(ret));
    }
    if (auto ret = pthread_attr_setinheritsched(&attr, PTHREAD_EXPLICIT_SCHED); ret != 0) {
        LLWFLOWS_LOG_ERROR("Failed to set thread inheritance, error: {}", strerror(ret));
    }
    if (mThreadMetaData) {
        pthread_t thread_handle = mThreadMetaData->native_handle();
        if (auto ret = pthread_setschedparam(thread_handle, policy, &param); ret != 0) {
            LLWFLOWS_LOG_ERROR("Failed to set thread({}) priority, error: {}", name(), strerror(ret));
        }
    }
}

auto Thread::run() -> void {
    if (mFunc) {
        mFunc();
    }
}

auto Thread::currentThread() -> Thread& {
    std::thread::id id = std::this_thread::get_id();
    auto            it = kThreads.find(id);
    if (it == kThreads.end()) {
        makeMetaObjectForCurrentThread(
            (std::string("unknown thread $") + std::to_string(std::hash<std::thread::id>()(id))).c_str());
        it = kThreads.find(id);
    }
    return *it->second;
}

auto Thread::makeMetaObjectForCurrentThread(const char* name) -> void {
    thread_local Thread metaObject;
    metaObject.mName         = name;
    metaObject.mFunc         = nullptr;
    metaObject.mId           = std::this_thread::get_id();
    kThreads[metaObject.mId] = &metaObject;
}

Thread::Thread() { auto [mPolicy, mPriority] = priority(); }

Thread::~Thread() {
    std::thread::id                              selfid;
    std::map<std::thread::id, Thread*>::iterator it;
    if (mThreadMetaData) {
        it = kThreads.find(mThreadMetaData->get_id());
    } else {
        it = kThreads.find(mId);
    }
    if (it != kThreads.end()) {
        kThreads.erase(it);
    }
}

LLWFLOWS_NS_END