#pragma once

#include <atomic>
#include <memory>

#include "detail/MPMCQueue.h"
#include "detail/log.hpp"
#include "detail/workflowsglobal.hpp"

LLWFLOWS_NS_BEGIN

template <typename T>
class SRingBuffer {
public:
    SRingBuffer(std::size_t size);
    ~SRingBuffer();
    bool push(T&& item);
    bool push(const T& item);
    template <typename... Args>
    bool        emplace(Args&&... args);
    bool        pop(T& item);
    bool        empty() const;
    std::size_t size() const;
    std::size_t capacity() const;

private:
    rigtorp::mpmc::Queue<T> mQueue;
};

template <typename T>
SRingBuffer<T>::SRingBuffer(std::size_t size) : mQueue(size) {}

template <typename T>
SRingBuffer<T>::~SRingBuffer() {}

template <typename T>
template <typename... Args>
inline bool SRingBuffer<T>::emplace(Args&&... args) {
    return mQueue.try_emplace(std::forward<Args>(args)...);
}

template <typename T>
bool SRingBuffer<T>::push(T&& item) {
    return mQueue.try_emplace(std::forward<T>(item));
}
template <typename T>
bool SRingBuffer<T>::push(const T& item) {
    return mQueue.try_emplace(item);
}

template <typename T>
bool SRingBuffer<T>::pop(T& item) {
    return mQueue.try_pop(item);
}

template <typename T>
inline bool SRingBuffer<T>::empty() const {
    return mQueue.empty();
}

template <typename T>
inline std::size_t SRingBuffer<T>::size() const {
    return mQueue.size();
}

template <typename T>
inline std::size_t SRingBuffer<T>::capacity() const {
    return mQueue.capacity();
}
LLWFLOWS_NS_END
