#pragma once
#include <atomic>
#include <chrono>
#include <functional>
#include <mutex>
#include <thread>

namespace sw::chrono {

class Timer
{
public:
    using clock = std::chrono::high_resolution_clock;

    ~Timer() { stop(); }

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

        const auto run = [interval, wait, callback, this]() {
            m_running = true;
            const auto sleepUntil = wait == Wait::Busy ? Timer::busyWait : Timer::sleepWait;
            while (m_running)
            {
                const auto wakeUpTime = clock::now() + interval;
                std::invoke(callback);
                sleepUntil(wakeUpTime);
            }
        };

        m_thread = std::thread(run);
        return true;
    }

    void stop()
    {
        std::lock_guard lock(m_startStopMutex);
        if (m_running)
        {
            m_running = false;
            m_thread.join();
        }
    }

    bool running() const { return m_running; }

private:
    static void busyWait(const clock::time_point wakeUpTime)
    {
        while (clock::now() < wakeUpTime)
            ;
    }
    static void sleepWait(const clock::time_point wakeUpTime) { std::this_thread::sleep_until(wakeUpTime); }

    std::mutex m_startStopMutex;
    std::thread m_thread;
    bool m_running = false;
};

}    // namespace sw::chrono
