#include <sw/chrono/stopwatch.hpp>
#include <sw/chrono/timer.hpp>

#include <gtest/gtest.h>

namespace sw::chrono::tests {

TEST(chronoTest, timer_with_busy_wait)
{
    sw::chrono::Timer timer;
    const size_t intervalMillisecs = 123u;
    const size_t waitFactor = 10u;
    const size_t waitMillisecs = waitFactor * intervalMillisecs;

    std::vector<Timer::clock::time_point> tickTimes;
    tickTimes.reserve(2 * waitFactor);

    const auto tick = [&tickTimes]() {
        tickTimes.push_back(Timer::clock::now());
        //        std::cout << "." << std::flush;
    };

    timer.start(tick, std::chrono::milliseconds(intervalMillisecs), Timer::Wait::Busy);
    std::this_thread::sleep_for(std::chrono::milliseconds(waitMillisecs));
    timer.stop();

    EXPECT_NEAR(tickTimes.size(), waitFactor, 1u);
    for (auto i = 0u; i + 1 < tickTimes.size(); ++i)
    {
        const auto diffMillisecs =
          std::chrono::duration_cast<std::chrono::milliseconds>(tickTimes[i + 1] - tickTimes[i]).count();
        //        std::cout << diffMillisecs << std::endl;
        EXPECT_NEAR(diffMillisecs, intervalMillisecs, 1u);
    }
}

TEST(chronoTest, timer_sleep_callback_takes_longer_than_interval)
{
    sw::chrono::Timer timer;
    size_t tickCount = 0u;
    const auto tick = [&tickCount]() {
        ++tickCount;
        std::this_thread::sleep_for(std::chrono::milliseconds(20));
    };

    timer.start(tick, std::chrono::milliseconds(10), Timer::Wait::Sleep);
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
    timer.stop();
    EXPECT_GT(tickCount, 1u);
}

}    // namespace sw::chrono::tests
