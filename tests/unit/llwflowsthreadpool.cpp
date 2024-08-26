#include <gtest/gtest.h>

#include <bitset>
#include <chrono>
#include <thread>

#include "../../workflows/log.hpp"
#include "../../workflows/threadpools.hpp"

LLWFLOWS_NS_USING

TEST(ThreadPoolTest, Basic) {
    constexpr int                num_test_threads = 10, num_test_tasks = 100;
    SRingBuffer<int>             doneIds(num_test_threads * num_test_tasks + 1);
    ThreadPool                   threadPool(num_test_threads);
    std::shared_ptr<TaskPromise> taskInfo[num_test_threads * num_test_tasks];
    threadPool.start();

    for (int i = 0; i < num_test_threads * num_test_tasks; ++i) {
        taskInfo[i] = threadPool.addTask([i, &doneIds]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(random() % 10));
            doneIds.push(i);
        });
    }

    threadPool.stopAndwaitAll();

    for (int i = 0; i < num_test_threads * num_test_tasks; ++i) {
        EXPECT_EQ(taskInfo[i]->workerId(), i % num_test_threads);
    }
    std::bitset<num_test_tasks * num_test_threads> tasks;
    int                                            id;
    while (doneIds.pop(id)) {
        EXPECT_EQ(tasks[id], 0);
        tasks[id] = true;
    }

    EXPECT_EQ(tasks.count(), num_test_tasks * num_test_threads);
}

TEST(ThreadPoolTest, Cancel) {
    constexpr int                num_test_threads = 10, num_test_tasks = 100;
    SRingBuffer<int>             doneIds(num_test_threads * num_test_tasks + 1);
    ThreadPool                   threadPool(num_test_threads);

    std::shared_ptr<TaskPromise> taskInfo[num_test_threads * num_test_tasks];
    for (int i = 0; i < num_test_threads * num_test_tasks; ++i) {
        taskInfo[i] = threadPool.addTask([i, &doneIds]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            doneIds.push(i);
        });
    }
    for (int i = 0; i < num_test_threads; ++i) {
        for (int j = 1; j < num_test_tasks; ++j) {
            taskInfo[i * num_test_tasks + j]->cancel();
        }
    }
    threadPool.stopAndwaitAll();
    std::bitset<num_test_tasks * num_test_threads> tasks;
    int                                            id;
    while (doneIds.pop(id)) {
        EXPECT_EQ(tasks[id], 0);
        tasks[id] = true;
    }

    EXPECT_EQ(tasks.count(), num_test_threads);
}

int main(int argc, char** argv) {
    Thread::makeMetaObjectForCurrentThread("main");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}