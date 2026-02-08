#include <sw/chrono/timer.hpp>
#include <sw/containers/lockfreeswap.hpp>
#include <sw/containers/utils.hpp>

#include <gtest/gtest.h>

namespace sw::containers::tests {

namespace {

template<size_t ArraySize>
struct TestStruct
{
    void push(int i)
    {
        swap.inSwap().fill(i);
        swap.push();
    }

    void pullAndCheck()
    {
        if (failedAnyway)
            return;
        const auto &consumedArray = swap.pull();
        const auto startValue = consumedArray.front();
        EXPECT_GE(startValue, lastValue);
        if (startValue < lastValue)
        {
            failedAnyway = true;
            return;
        }
        for (const auto i : consumedArray)
        {
            EXPECT_EQ(i, startValue);
            if (i != startValue)
            {
                failedAnyway = true;
                break;
            }
        }
        lastValue = startValue;
    }

    using Array = std::array<int, ArraySize>;
    int lastValue = 0;
    bool failedAnyway = false;
    LockFreeSwap<Array> swap{makeFilledArray<ArraySize>(lastValue)};
};

}    // namespace

TEST(LockFreeSwapTest, slower_feeding)
{
    TestStruct<100> test{};

    chrono::Timer feedTimer;
    int i = 0;
    feedTimer.start([&]() { test.push(++i); }, std::chrono::milliseconds(3), chrono::Timer::Wait::Busy);

    bool keepConsuming = true;
    std::thread consumingThread([&]() {
        while (keepConsuming)
            test.pullAndCheck();
    });

    std::this_thread::sleep_for(std::chrono::seconds(3));
    keepConsuming = false;
    feedTimer.stop();
    consumingThread.join();
}

TEST(LockFreeSwapTest, slower_consuming)
{
    TestStruct<50> test{};

    bool keepFeeding = true;
    std::thread feedingThread([&]() {
        auto i = 0;
        while (keepFeeding)
            test.push(++i);
    });

    chrono::Timer consumeTimer;
    consumeTimer.start([&]() { test.pullAndCheck(); }, std::chrono::milliseconds(3), chrono::Timer::Wait::Busy);

    std::this_thread::sleep_for(std::chrono::seconds(3));
    keepFeeding = false;
    consumeTimer.stop();
    feedingThread.join();
}

TEST(LockFreeSwapTest, fast_feeding_and_consuming)
{
    TestStruct<20> test{};

    bool keepFeeding = true;
    std::thread feedingThread([&]() {
        int i = 0;
        while (keepFeeding)
            test.push(++i);
    });

    bool keepConsuming = true;
    std::thread consumingThread([&]() {
        while (keepConsuming)
            test.pullAndCheck();
    });

    std::this_thread::sleep_for(std::chrono::seconds(3));
    keepFeeding = false;
    keepConsuming = false;
    feedingThread.join();
    consumingThread.join();
}

}    // namespace sw::containers::tests
