#pragma once

#include <atomic>
#include <memory>

#include "workflowsglobal.hpp"

LLWFLOWS_NS_BEGIN

template <typename T>
class SRingBuffer {
public:
    SRingBuffer(std::size_t size);
    ~SRingBuffer();
    void        resize(std::size_t size);
    void        push(T item);
    T           pop();
    bool        empty() const;
    std::size_t size() const;
    void        clear();
    T&          front();
    const T&    front() const;
    T&          back();
    const T&    back() const;

private:
    std::unique_ptr<T> mQueue;
    std::atomic<int>   mHead;
    std::atomic<int>   mTail;
    std::atomic<int>   mSize;
};

template <typename T>
SRingBuffer<T>::SRingBuffer(std::size_t size) : mQueue(new T[size]), mHead(0), mTail(0), mSize(size) {}

template <typename T>
SRingBuffer<T>::~SRingBuffer() {}

template <typename T>
void SRingBuffer<T>::resize(std::size_t size) {
    mQueue.reset(new T[size]);
    mHead = 0;
    mTail = 0;
    mSize = size;
}

template <typename T>
void SRingBuffer<T>::push(T item) {

}

LLWFLOWS_NS_END