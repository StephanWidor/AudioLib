#pragma once
#include <array>
#include <atomic>

namespace sw::containers {

/// Lockfree triple swap for single producer single consumer
template<std::copyable ValueType>
class LockFreeSwap
{
public:
    using Value = ValueType;

    LockFreeSwap(const Value &initValue): m_data{{initValue, initValue, initValue}} {}

    // feeding/producing side

    /// Get a reference to the current value of the producer side. After being done with manipulating, call push() to
    /// get data to the consuming side.
    /// ATTENTION: Using this reference after calling push will lead to undefined behavior!
    Value &inSwap()
    {
        if (m_writingPage > 2)
        {
            for (auto i = 0u; i < 3; ++i)
            {
                DataState expectedState = Written;
                if (std::atomic_ref(m_dataStates[i]).compare_exchange_weak(expectedState, Writing))
                {
                    m_writingPage = i;
                    return m_data[i];
                }
            }
            for (auto i = 0u; true; i = (i + 1) % 3)
            {
                if (std::atomic_ref(m_dataStates[i]).load(std::memory_order::acquire) == Free)
                {
                    m_dataStates[i] = Writing;
                    m_writingPage = i;
                    return m_data[i];
                }
            }
        }
        return m_data[m_writingPage];
    }

    /// Set a value on producer side. After this, call push() to get data to the consuming side.
    void set(const Value &value) { inSwap() = value; }

    /// Push data from producer to consumer side
    void push()
    {
        if (m_writingPage < 3)
        {
            std::atomic_ref(m_dataStates[m_writingPage]).store(Written, std::memory_order::release);
            m_writingPage = 3;
        }
    }

    // consumer side

    /// Get a reference to the current value of the consumer side.
    /// ATTENTION: This reference stays valid, no matter what happens on producer side (though it will stay the same, not
    /// containing new data that has been pushed). But once you have called pull again, you should use the reference
    /// returned by that call, and throw away this one to avoid undefined behavior.
    const Value &pull() const
    {
        for (auto i = 0u; i < 3; ++i)
        {
            DataState expectedState = Written;
            if (std::atomic_ref(m_dataStates[i]).compare_exchange_weak(expectedState, Reading))
            {
                std::atomic_ref(m_dataStates[m_readingPage]).store(Free, std::memory_order::release);
                m_readingPage = i;
                break;
            }
        }
        return m_data[m_readingPage];
    }

private:
    enum DataState : std::uint8_t
    {
        Free,
        Writing,
        Written,
        Reading
    };

    std::uint8_t m_writingPage = 0u;
    mutable std::uint8_t m_readingPage = 1u;
    mutable std::array<DataState, 3> m_dataStates{DataState::Writing, DataState::Reading, DataState::Free};
    std::array<Value, 3> m_data;
};

}    // namespace sw::containers
