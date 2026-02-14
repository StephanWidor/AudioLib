#include <sw/chrono/timer.hpp>
#include <sw/containers/spsc/spinlockedringbuffer.hpp>

#include <gtest/gtest.h>

#include <thread>

namespace sw::containers::spsc::tests {

void checkBuffer(const size_t bufferSize, const std::chrono::milliseconds feedInterval,
                 const std::chrono::milliseconds consumeInterval)
{
    SpinLockedRingBuffer<size_t> buffer(bufferSize, 0u);
    size_t feedCount = 0u;
    std::vector<size_t> consumed(bufferSize, 0u);

    const auto feed = [&]() { buffer.push(++feedCount); };
    const auto consume = [&]() { consumed = buffer.outBuffer(); };

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

    EXPECT_EQ(buffer.inBuffer(), consumed);

    const auto checkIncrement = [&]() {
        for (auto i = 1u; i < bufferSize; ++i)
        {
            const auto last = consumed[i - 1];
            const auto v = consumed[i];
            if ((last != 0u || v != 0u) && v != last + 1)
                return false;
        }
        return true;
    };

    EXPECT_TRUE(checkIncrement());
}

TEST(SpinLockedBufferTest, faster_feeding)
{
    checkBuffer(100u, std::chrono::milliseconds(5), std::chrono::milliseconds(21));
}

TEST(SpinLockedBufferTest, faster_consuming)
{
    checkBuffer(20u, std::chrono::milliseconds(40), std::chrono::milliseconds(11));
}

}    // namespace sw::containers::spsc::tests
