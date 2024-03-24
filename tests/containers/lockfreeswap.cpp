#include <sw/chrono/timer.hpp>
#include <sw/containers/lockfreeswap.hpp>

#include <gtest/gtest.h>

#include <thread>

namespace sw::containers::tests {

namespace {

void checkSwap(const std::chrono::milliseconds feedInterval, const std::chrono::milliseconds consumeInterval)
{
    using TestPair = std::pair<int, double>;
    LockFreeSwap<TestPair> swap({});

    TestPair feedPair{12, 1.1};
    const auto feed = [&]() {
        feedPair.first += 1;
        feedPair.second *= 1.7;
        swap.set(feedPair);
        swap.push();
    };

    TestPair consumePair{2, 0.1};
    const auto consume = [&]() { consumePair = swap.pull(); };

    chrono::Timer feedTimer, consumeTimer;

    std::thread feedingThread([&]() { feedTimer.start(feed, feedInterval, chrono::Timer::Wait::Busy); });
    std::thread consumingThread([&]() { consumeTimer.start(consume, consumeInterval, chrono::Timer::Wait::Busy); });

    std::this_thread::sleep_for(
      std::chrono::milliseconds(100 * std::max(feedInterval.count(), consumeInterval.count())));

    feedTimer.stop();
    feedingThread.join();

    std::this_thread::sleep_for(consumeInterval);

    consumeTimer.stop();
    consumingThread.join();

    EXPECT_EQ(feedPair, consumePair);
}

}    // namespace

TEST(LockFreeSwapTest, faster_feeding)
{
    checkSwap(std::chrono::milliseconds(5), std::chrono::milliseconds(21));
}

TEST(LockFreeSwapTest, faster_consuming)
{
    checkSwap(std::chrono::milliseconds(40), std::chrono::milliseconds(11));
}

}    // namespace sw::containers::tests
