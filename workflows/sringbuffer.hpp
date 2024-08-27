#pragma once

#include <atomic>
#include <memory>

#include "workflowsglobal.hpp"

LLWFLOWS_NS_BEGIN

template <typename T>
class SRingBuffer {
    struct Element {
        T                 value;
        std::atomic<bool> full;
    };

public:
    SRingBuffer(std::size_t size);
    ~SRingBuffer();
    void        resize(std::size_t size);
    bool        push(T&& item);
    bool        push(const T& item);
    bool        pop(T& item);
    bool        empty() const;
    std::size_t size() const;
    std::size_t capacity() const;
    /**
     * @brief clear all element
     * this function is not thread safety
     */
    void clear();

private:
    std::unique_ptr<Element[]> mQueue;
    std::atomic<std::size_t>   mHead;
    std::atomic<std::size_t>   mTail;
    std::atomic<std::size_t>   mSize;
    std::size_t                mCapacity;
};

template <typename T>
SRingBuffer<T>::SRingBuffer(std::size_t size)
    : mQueue(new Element[size]), mHead(0), mTail(0), mSize(0), mCapacity(size) {}

template <typename T>
SRingBuffer<T>::~SRingBuffer() {}

template <typename T>
void SRingBuffer<T>::resize(std::size_t size) {
    mQueue.reset(new T[size]);
    mHead = 0;
    mTail = 0;
    mSize = 0;
}
template <typename T>
bool SRingBuffer<T>::push(const T& item) {
    std::size_t tail = 0, index = 0, size = 0;
    Element*    e = nullptr;
    do {
        size = mSize.load(std::memory_order_relaxed);
        if (size + 1 > mCapacity) {
            return false;
        }
    } while (!mSize.compare_exchange_weak(size, size + 1, std::memory_order_release, std::memory_order_relaxed));
    while (true) {
        tail  = mTail.load(std::memory_order_relaxed);
        index = (tail + 1) % mCapacity;
        e     = mQueue.get() + tail;
        if (e->full.load(std::memory_order_relaxed)) {
            continue;
        }
        if (mTail.compare_exchange_weak(tail, index, std::memory_order_release, std::memory_order_relaxed)) {
            break;
        }
    }
    e->value = item;
    e->full.store(true, std::memory_order_release);

    return true;
}

template <typename T>
bool SRingBuffer<T>::push(T&& item) {
    std::size_t tail = 0, index = 0, size = 0;
    Element*    e = nullptr;
    size          = mSize.load(std::memory_order_relaxed);
    do {
        if (size + 1 > mCapacity) {
            return false;
        }
    } while (!mSize.compare_exchange_weak(size, size + 1, std::memory_order_release, std::memory_order_relaxed));
    while (true) {
        tail  = mTail.load(std::memory_order_relaxed);
        index = (tail + 1) % mCapacity;
        e     = mQueue.get() + tail;
        if (e->full.load(std::memory_order_relaxed)) {
            continue;
        }
        if (mTail.compare_exchange_weak(tail, index, std::memory_order_release, std::memory_order_relaxed)) {
            break;
        }
    }
    e->value = std::move(item);
    e->full.store(true, std::memory_order_release);

    return true;
}

template <typename T>
bool SRingBuffer<T>::pop(T& item) {
    std::size_t head = 0, index = 0;
    Element*    e = nullptr;
    head          = mHead.load(std::memory_order_relaxed);
    do {
        if (mSize.load(std::memory_order_relaxed) == 0) {
            return false;
        }
        index = (head + 1) % mCapacity;
        e     = mQueue.get() + head;
        if (!e->full.load(std::memory_order_relaxed)) {
            return false;
        }
    } while (!mHead.compare_exchange_weak(head, index, std::memory_order_release, std::memory_order_relaxed));

    item = std::move(e->value);
    e->full.store(false, std::memory_order_release);
    mSize.fetch_sub(1, std::memory_order_release);

    return true;
}

template <typename T>
inline bool SRingBuffer<T>::empty() const {
    return mSize.load(std::memory_order_release) == 0;
}

template <typename T>
inline std::size_t SRingBuffer<T>::size() const {
    return mSize.load(std::memory_order_release);
}

template <typename T>
inline std::size_t SRingBuffer<T>::capacity() const {
    return mCapacity;
}

template <typename T>
inline void SRingBuffer<T>::clear() {
    mTail.store(0, std::memory_order_relaxed);
    mSize.store(0, std::memory_order_relaxed);
    mHead.store(0, std::memory_order_relaxed);
}

LLWFLOWS_NS_END