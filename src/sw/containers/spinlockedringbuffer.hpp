#pragma once
#include "sw/containers/utils.hpp"

#include <atomic>
#include <vector>

namespace sw::containers {

template<typename ValueType>
class SpinLockedRingBuffer
{
public:
    SpinLockedRingBuffer(const size_t size, const ValueType initValue)
        : m_inBuffer(size, initValue), m_outBuffer(size, initValue)
    {}

    void reset(const size_t size, const ValueType initValue = static_cast<ValueType>(0))
    {
        m_inBuffer.resize(0);
        m_inBuffer.resize(size, initValue);
        m_outBuffer.resize(0);
        m_outBuffer.resize(size, initValue);
    }

    /// ring push data from feeding thread
    void push(const ValueType &value)
    {
        lock();
        ringPush(m_inBuffer, value);
        m_newDataAvailable = true;
        unlock();
    }

    /// ring push data from feeding thread
    template<ranges::TypedInputRange<ValueType> R>
    void push(R &&values)
    {
        lock();
        ringPush(m_inBuffer, std::forward<R>(values));
        m_newDataAvailable = true;
        unlock();
    }

    /// only call from feeding thread
    const std::vector<ValueType> &inBuffer() const { return m_inBuffer; }

    /// get data from consumer thread (thread safe in respect of pushing/setting/changing data from feeding thread,
    /// as long as there is only one consumer thread)
    const std::vector<ValueType> &outBuffer() const
    {
        if (m_newDataAvailable)
        {
            lock();
            std::copy(m_inBuffer.begin(), m_inBuffer.end(), m_outBuffer.begin());
            m_newDataAvailable = false;
            unlock();
        }
        return m_outBuffer;
    }

private:
    void lock() const
    {
        while (m_locked.test_and_set(std::memory_order_acquire))
            ;
    }
    void unlock() const { m_locked.clear(std::memory_order_release); }

    std::vector<ValueType> m_inBuffer;
    mutable std::vector<ValueType> m_outBuffer;
    mutable std::atomic_flag m_locked;
    mutable std::atomic<bool> m_newDataAvailable{true};
};

}    // namespace sw::containers
