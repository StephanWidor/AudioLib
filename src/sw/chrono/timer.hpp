#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <thread>

namespace sw::chrono {

class Timer
{
public:
    using clock = std::chrono::high_resolution_clock;

    ~Timer() { stop(); }

    void interval(std::chrono::milliseconds interval) { m_interval = interval; }

    std::chrono::milliseconds interval() const { return m_interval; }

    enum class Wait
    {
        Sleep,
        Busy
    };

    bool start(std::invocable auto &&callback, std::chrono::milliseconds interval, Wait wait)
    {
        std::lock_guard lock(m_startStopMutex);
        if (m_running)
            return false;

        const auto run = [&]() {
            bool running = true;
            m_running = &running;
            const auto sleepUntil = wait == Wait::Busy ? Timer::busyWait : Timer::sleepWait;
            while (running)
            {
                const auto wakeUpTime = clock::now() + m_interval.load();
                std::invoke(callback);
                sleepUntil(wakeUpTime);
            }
        };

        m_interval = interval;
        std::thread(run).detach();
        return true;
    }

    void stop()
    {
        std::lock_guard lock(m_startStopMutex);
        if (m_running)
        {
            *m_running = false;
            m_running = nullptr;
        }
    }

    bool running() const { return m_running && *m_running; }

private:
    static void busyWait(const clock::time_point wakeUpTime)
    {
        while (clock::now() < wakeUpTime)
            ;
    }
    static void sleepWait(const clock::time_point wakeUpTime) { std::this_thread::sleep_until(wakeUpTime); }

    std::atomic<std::chrono::milliseconds> m_interval{std::chrono::milliseconds{1000}};
    std::mutex m_startStopMutex;
    bool *m_running = nullptr;
};

}    // namespace sw::chrono
