#include <gtest/gtest.h>

#include <bitset>
#include <chrono>
#include <thread>

#include "../../workflows/log.hpp"
#include "../../workflows/threadworker.hpp"

LLWFLOWS_NS_USING

TEST(ThreadWorkerTest, Basic) {
    constexpr int                num_test_threads = 10, num_test_tasks = 100;
    SRingBuffer<int>             doneIds(num_test_threads * num_test_tasks + 1);
    ThreadWorker                 threads[num_test_threads];
    std::shared_ptr<TaskPromise> taskInfo[num_test_threads * num_test_tasks];
    for (int i = 0; i < num_test_threads; ++i) {
        threads[i].init(i);
        threads[i].start();
        for (int j = 0; j < num_test_tasks; ++j) {
            taskInfo[i * num_test_tasks + j] = threads[i].post([i, j, &doneIds]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(random() % 10));
                doneIds.push(i * num_test_tasks + j);
            });
        }
    }
    for (int i = 0; i < num_test_threads; ++i) threads[i].exit(true);
    for (int i = 0; i < num_test_threads; ++i) threads[i].waitForExit();
    for (int i = 0; i < num_test_threads; ++i) {
        for (int j = 0; j < num_test_tasks; ++j) {
            EXPECT_EQ(taskInfo[i * num_test_tasks + j]->workerId(), i);
        }
    }
    std::bitset<num_test_tasks * num_test_threads> tasks;
    int                                            id;
    while (doneIds.pop(id)) {
        EXPECT_EQ(tasks[id], 0);
        tasks[id] = true;
    }

    EXPECT_EQ(tasks.count(), num_test_tasks * num_test_threads);
}

TEST(ThreadWorkerTest, Cancel) {
    constexpr int                num_test_threads = 10, num_test_tasks = 100;
    SRingBuffer<int>             doneIds(num_test_threads * num_test_tasks + 1);
    ThreadWorker                 threads[num_test_threads];
    std::shared_ptr<TaskPromise> taskInfo[num_test_threads * num_test_tasks];
    for (int i = 0; i < num_test_threads; ++i) {
        threads[i].init(i);
        threads[i].start();
        for (int j = 0; j < num_test_tasks; ++j) {
            taskInfo[i * num_test_tasks + j] = threads[i].post([i, j, &doneIds]() {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                doneIds.push(i * num_test_tasks + j);
            });
        }
    }
    for (int i = 0; i < num_test_threads; ++i) {
        for (int j = 1; j < num_test_tasks; ++j) {
            taskInfo[i * num_test_tasks + j]->cancel();
        }
        threads[i].exit(true);
    }
    for (int i = 0; i < num_test_threads; ++i) threads[i].waitForExit();
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