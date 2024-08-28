#include <gtest/gtest.h>

#include <bitset>
#include <thread>

#include "../../workflows/detail/log.hpp"
#include "../../workflows/sringbuffer.hpp"

LLWFLOWS_NS_USING

TEST(RingBufferTest, Basic) {
    SRingBuffer<int> buffer(100);

    for (int i = 0; i < 100; ++i) {
        EXPECT_TRUE(buffer.push(i));
    }

    for (int i = 0; i < 100; ++i) {
        int t = 0;
        EXPECT_TRUE(buffer.pop(t));
        EXPECT_EQ(t, i);
    }
}

TEST(RingBufferTest, MultiThreadWrite) {
    SRingBuffer<int> buffer(10);

    std::thread      threads[20];
    for (int i = 0; i < 10; ++i) {
        threads[i] = std::thread([&buffer, count = i]() {
            LLWFLOWS_LOG_INFO("thread {} start.", count);
            for (int j = 0; j < 10; ++j) {
                while (!buffer.push(j + count * 10)) {
                };
            }
        });
    }

    SRingBuffer<int> buffer1(100);
    for (int i = 10; i < 20; ++i) {
        threads[i] = std::thread([&buffer, &buffer1, count = i]() {
            LLWFLOWS_LOG_INFO("thread {} start.", count);
            for (int j = 0; j < 10; ++j) {
                int t = 0;
                while (!buffer.pop(t)) {
                };
                if (t >= 100) LLWFLOWS_LOG_WARN("t = {}", t);
                buffer1.push(t);
            }
        });
    }

    for (int i = 0; i < 20; ++i) {
        threads[i].join();
    }
    std::bitset<100> bits;
    while (buffer1.size() > 0) {
        int t = 0;
        buffer1.pop(t);
        ASSERT_LT(t, 100);
        EXPECT_EQ(bits[t], 0) << "t = " << t;
        bits[t] = 1;
    }
    EXPECT_TRUE(bits.all());
}

int main(int argc, char** argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}