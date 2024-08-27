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

    std::bitset<num_test_tasks * num_test_threads> tasks;
    int                                            id;
    while (doneIds.pop(id)) {
        EXPECT_EQ(tasks[id], 0);
        tasks[id] = true;
    }

    EXPECT_EQ(tasks.count(), num_test_tasks * num_test_threads);
}

TEST(ThreadPoolTest, Cancel) {
    constexpr int    num_test_threads = 10, num_test_tasks = 100;
    SRingBuffer<int> doneIds(num_test_threads * num_test_tasks + 1);
    ThreadPool       threadPool(num_test_threads);
    threadPool.start();

    std::shared_ptr<TaskPromise> taskInfo[num_test_threads * num_test_tasks];
    for (int i = 0; i < num_test_threads; ++i) {
        for (int j = 0; j < num_test_tasks; ++j) {
            TaskDescription desc;
            desc.specifyWorkerId             = i;
            taskInfo[i * num_test_tasks + j] = threadPool.addTask(
                [idx = i * num_test_tasks + j, &doneIds]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
                    doneIds.push(idx);
                },
                desc);
        }
    }
    for (int i = 0; i < num_test_threads; ++i) {
        for (int j = 1; j < num_test_tasks; ++j) {
            if (taskInfo[i * num_test_tasks + j]->cancel() != 0)
                LLWFLOWS_LOG_WARN("Task {} cancel failed", i * num_test_tasks + j);
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

TEST(ThreadPoolTest, InvalidWorkerId) {
    ThreadPool threadPool(10);
    threadPool.start();

    TaskDescription desc;
    desc.specifyWorkerId = 11;
    EXPECT_TRUE(threadPool.addTask([]() { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); }, desc) ==
                nullptr);
    desc.specifyWorkerId = 12;
    EXPECT_TRUE(threadPool.addTask([]() { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); }, desc) ==
                nullptr);
    desc.specifyWorkerId = -2;
    EXPECT_TRUE(threadPool.addTask([]() { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); }, desc) ==
                nullptr);
    desc.specifyWorkerId = 10;
    EXPECT_TRUE(threadPool.addTask([]() { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); }, desc) ==
                nullptr);
    desc.specifyWorkerId = 9;
    EXPECT_TRUE(threadPool.addTask([]() { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); }, desc) !=
                nullptr);
    desc.specifyWorkerId = 0;
    EXPECT_TRUE(threadPool.addTask([]() { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); }, desc) !=
                nullptr);
    desc.specifyWorkerId = 5;
    EXPECT_TRUE(threadPool.addTask([]() { std::this_thread::sleep_for(std::chrono::milliseconds(1000)); }, desc) !=
                nullptr);
    threadPool.stop();
}

TEST(ThreadPoolTest, waitTask) {
    constexpr int                num_test_threads = 10, num_test_tasks = 10;
    SRingBuffer<int>             doneIds(num_test_threads * num_test_tasks + 1);
    ThreadPool                   threadPool(num_test_threads);
    std::shared_ptr<TaskPromise> taskInfo[num_test_threads * num_test_tasks];
    threadPool.start();

    for (int i = 0; i < num_test_threads * num_test_tasks; ++i) {
        taskInfo[i] = threadPool.addTask([i, &doneIds]() {
            std::this_thread::sleep_for(std::chrono::milliseconds(random() % 100));
            doneIds.push(i);
        });
    }

    for (int i = 0; i < num_test_threads * num_test_tasks; ++i) {
        threadPool.wait(taskInfo[i]);
    }

    threadPool.stop();

    std::bitset<num_test_tasks * num_test_threads> tasks;
    int                                            id;
    while (doneIds.pop(id)) {
        EXPECT_EQ(tasks[id], 0);
        tasks[id] = true;
    }

    EXPECT_EQ(tasks.count(), num_test_tasks * num_test_threads);
};

TEST(ThreadPoolTest, depends) {
    constexpr int                num_test_threads = 10, num_test_tasks = 100;
    SRingBuffer<int>             doneIds(num_test_threads * num_test_tasks + 1);
    ThreadPool                   threadPool(num_test_threads);
    std::shared_ptr<TaskPromise> taskInfo[num_test_threads * num_test_tasks];
    threadPool.start();
    int count = 0;
    for (int i = 0; i < num_test_threads * num_test_tasks; ++i) {
        TaskDescription desc;
        desc.name = "task-" + std::to_string(i);
        if (i > 0) {
            desc.dependencies.push_back(taskInfo[i - 1]);
        }
        taskInfo[i] = threadPool.addTask(
            [&count, &doneIds, &taskInfo, i = i]() {
                doneIds.push(count);
                ++count;
            },
            desc);
    }

    threadPool.wait(taskInfo[num_test_threads * num_test_tasks - 1]);
    threadPool.stopAndwaitAll();
    std::bitset<num_test_tasks * num_test_threads> counts;
    int                                            id;
    while (doneIds.pop(id)) {
        EXPECT_EQ(counts[id], 0);
        EXPECT_EQ(id, counts.count());
        counts[id] = true;
    }

    EXPECT_EQ(counts.count(), num_test_tasks * num_test_threads);
};

TEST(ThreadPoolTest, priority) {
    constexpr int                          num_test_threads = 5, num_test_tasks = 3000;
    SRingBuffer<std::pair<int, long long>> doneIds(num_test_threads * num_test_tasks + 1);
    ThreadPool                             threadPool(num_test_threads);
    std::shared_ptr<TaskPromise>           taskInfo[num_test_threads * num_test_tasks];
    threadPool.start();
    for (int i = 0; i < num_test_tasks / 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            TaskDescription desc;
            desc.priority       = (TaskPriority)(j);
            desc.name           = "task-" + std::to_string(i * 3 + j);
            clock_t tStrat      = clock();
            taskInfo[i * 3 + j] = threadPool.addTask(
                [&doneIds, j, tStrat]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    doneIds.push(std::make_pair(j, clock() - tStrat));
                },
                desc);
        }
    }

    threadPool.stopAndwaitAll();
    std::vector<uint64_t>     counts(3, 0);
    std::pair<int, long long> id;
    while (doneIds.pop(id)) {
        counts[id.first] += id.second;
    }

    EXPECT_GE(counts[1], counts[0]);
    EXPECT_GE(counts[2], counts[1]);
};

int main(int argc, char** argv) {
    Thread::makeMetaObjectForCurrentThread("main");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}