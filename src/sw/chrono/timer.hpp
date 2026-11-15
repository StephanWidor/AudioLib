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
    using clock = std::chrono::steady_clock;

    ~Timer() { stop(); }

    enum class Wait
    {
        Sleep,
        Busy
    };

    // Start a timer callback.
    // Callbacks may call requestStop() and running(), but they must not call start(), stop(), or destroy this Timer.
    // Exceptions escaping a callback terminate the application.
    bool start(std::invocable auto &&callback, std::chrono::milliseconds interval, Wait wait)
    {
        std::lock_guard lock(m_startStopMutex);
        auto expectedRunState = RunState::Stopped;
        if (!m_runState.compare_exchange_strong(expectedRunState, RunState::Running, std::memory_order::release,
                                                std::memory_order::relaxed))
            return false;

        if (m_thread.joinable())
            m_thread.join();
        try
        {
            m_thread = std::thread([&runState = m_runState, interval, wait, callback]() {
                const auto sleepUntil = wait == Wait::Busy ? Timer::busyWait : Timer::sleepWait;
                while (runState.load(std::memory_order::acquire) == RunState::Running)
                {
                    const auto wakeUpTime = clock::now() + interval;
                    std::invoke(callback);
                    if (runState.load(std::memory_order::acquire) != RunState::Running)
                        break;
                    sleepUntil(wakeUpTime);
                }
                runState.store(RunState::Stopped, std::memory_order::release);
            });
        }
        catch (...)
        {
            m_runState.store(RunState::Stopped, std::memory_order::release);
            throw;
        }
        return true;
    }

    // Request a stop. Will return immediately, not waiting for the timer thread to be finished.
    // May be called from within the callback.
    void requestStop()
    {
        auto expectedRunState = RunState::Running;
        m_runState.compare_exchange_strong(expectedRunState, RunState::Stopping, std::memory_order::release,
                                           std::memory_order::relaxed);
    }

    // Request a stop and wait until timer thread has finished.
    // Must not be called from within the callback!
    void stop()
    {
        std::lock_guard lock(m_startStopMutex);
        requestStop();
        if (m_thread.joinable())
            m_thread.join();
    }

    bool running() const { return m_runState.load(std::memory_order::acquire) != RunState::Stopped; }

private:
    static void busyWait(const clock::time_point wakeUpTime)
    {
        while (clock::now() < wakeUpTime)
            ;
    }
    static void sleepWait(const clock::time_point wakeUpTime) { std::this_thread::sleep_until(wakeUpTime); }

    enum class RunState
    {
        Stopped,
        Running,
        Stopping
    };

    std::atomic<RunState> m_runState{RunState::Stopped};
    std::thread m_thread;
    std::mutex m_startStopMutex;
};

}    // namespace sw::chrono
