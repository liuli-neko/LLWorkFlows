#include <gtest/gtest.h>

#include <bitset>
#include <thread>

#include "../../workflows/detail/log.hpp"
#include "../../workflows/thread.hpp"

LLWFLOWS_NS_USING

TEST(ThreadTest, Basic) {
    Thread threads[10];
    LLWFLOWS_LOG_INFO("policy : {}, priority: {}, max priority: {}", threads[0].priority().first,
                      threads[0].priority().second, threads[0].maxPriority());
    for (int i = 0; i < 10; ++i) {
        threads[i].setName((std::string("Thread-") + std::to_string(i)).c_str());
        threads[i].setPriority(Thread::SchedRoundRobin, i + 1);
        threads[i].start([id = i, &threads]() {
            char name[254];
            memset(name, 0, sizeof(name));
            pthread_getname_np(pthread_self(), name, sizeof(name));
            EXPECT_STREQ((std::string("Thread-") + std::to_string(id)).c_str(), threads[id].name());
            EXPECT_STREQ((std::string("Thread-") + std::to_string(id)).c_str(), name);
            auto priority = threads[id].priority();
            LLWFLOWS_LOG_INFO("runing thread name: {}, sched policy: {}, priority: {}", name, priority.first, priority.second);
        });
    }
    for (int i = 0; i < 10; ++i) {
        threads[i].join();
    }
}

int main(int argc, char** argv) {
    Thread::makeMetaObjectForCurrentThread("main");
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}