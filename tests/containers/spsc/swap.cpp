#include <sw/chrono/timer.hpp>
#include <sw/containers/spsc/swap.hpp>
#include <sw/containers/utils.hpp>

#include <gtest/gtest.h>

namespace sw::containers::spsc::tests {

namespace {

void testFeedingAndConsuming(std::chrono::milliseconds feedingWaitTime, std::chrono::milliseconds consumingWaitTime,
                             std::chrono::milliseconds runTime)
{
    const size_t ArraySize = 50u;
    using Array = std::array<int, ArraySize>;
    int lastValue = 0;
    bool failedAnyway = false;
    Swap<Array> swap{makeFilledArray<ArraySize>(lastValue)};
    bool keepRunning = true;

    std::thread feedingThread([&]() {
        int i = 0;
        while (keepRunning)
        {
            swap.inSwap().fill(++i);
            swap.push();
            if (feedingWaitTime != std::chrono::milliseconds::zero())
                std::this_thread::sleep_for(feedingWaitTime);
        }
    });

    std::thread consumingThread([&]() {
        while (keepRunning)
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
            if (consumingWaitTime != std::chrono::milliseconds::zero())
                std::this_thread::sleep_for(consumingWaitTime);
        }
    });

    std::this_thread::sleep_for(runTime);
    keepRunning = false;
    feedingThread.join();
    consumingThread.join();
}

}    // namespace

TEST(SpscSwapTest, slower_feeding)
{
    testFeedingAndConsuming(std::chrono::milliseconds(10), std::chrono::milliseconds::zero(),
                            std::chrono::milliseconds(3000));
}

TEST(SpscSwapTest, slower_consuming)
{
    testFeedingAndConsuming(std::chrono::milliseconds::zero(), std::chrono::milliseconds(10),
                            std::chrono::milliseconds(3000));
}

TEST(SpscSwapTest, fast_feeding_and_consuming)
{
    testFeedingAndConsuming(std::chrono::milliseconds::zero(), std::chrono::milliseconds::zero(),
                            std::chrono::milliseconds(3000));
}

}    // namespace sw::containers::spsc::tests
