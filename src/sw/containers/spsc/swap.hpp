#pragma once
#include <array>
#include <atomic>

namespace sw::containers::spsc {

/// Lockfree triple swap for single producer single consumer
template<std::copyable ValueType>
class Swap
{
public:
    using Value = ValueType;

    Swap(const Value &initValue): m_data{{initValue, initValue, initValue}} {}

    // feeding/producing side

    /// Get a reference to the current value of the producer side.
    /// After being done with manipulating the reference, you still need to call push() to get it over to the consumer side.
    /// ATTENTION: Using this reference after calling push will lead to undefined behavior!
    Value &inSwap()
    {
        if (m_writingPage < 3)
            return m_data[m_writingPage];

        // If we find a free page, we will use that as new writing page
        for (auto i = 0; i < 3; ++i)
        {
            if (std::atomic_ref(m_dataStates[i]).load(std::memory_order::acquire) == Free)
            {
                m_dataStates[i] = Writing;
                m_writingPage = i;
                return m_data[i];
            }
        }

        // We haven't found a free page (i.e. written page hasn't been consumed yet), so let's see if we can reuse the last written page
        //const auto last_written_page = m_writtenPage;
        DataState expectedState = Written;
        if (std::atomic_ref(m_dataStates[m_writtenPage])
              .compare_exchange_weak(expectedState, Writing, std::memory_order::acq_rel, std::memory_order::acquire))
        {
            m_writingPage = m_writtenPage;
            return m_data[m_writingPage];
        }

        // We can still get here, if consumer grabbed the written page just before we wanted to reuse it.
        // In that case, the consumer will free the old reading page within its current pull, so we just loop until that happened.
        for (auto i = 0u; true; i = (i + 1) % 3)
        {
            if (std::atomic_ref(m_dataStates[i]).load(std::memory_order::acquire) == Free)
            {
                m_dataStates[i] = Writing;
                m_writingPage = i;
                return m_data[i];
            }
        }
        return m_data[m_writingPage];
    }

    /// Set a new producer value.
    /// After this, you still need to call push() to get it over to the consumer side.
    void set(const Value &value) { inSwap() = value; }

    /// Push data from producer to consumer side
    void push()
    {
        if (m_writingPage > 2)
            return;

        DataState expectedState = Written;
        std::atomic_ref(m_dataStates[m_writtenPage])
          .compare_exchange_weak(expectedState, Free, std::memory_order::acq_rel, std::memory_order::acquire);
        std::atomic_ref(m_dataStates[m_writingPage]).store(Written, std::memory_order::release);
        m_writtenPage = m_writingPage;
        m_writingPage = 3;
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
            if (std::atomic_ref(m_dataStates[i])
                  .compare_exchange_weak(expectedState, Reading, std::memory_order::acq_rel,
                                         std::memory_order::acquire))
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
    std::uint8_t m_writtenPage = 1u;
    mutable std::uint8_t m_readingPage = 1u;
    mutable std::array<DataState, 3> m_dataStates{DataState::Writing, DataState::Reading, DataState::Free};
    std::array<Value, 3> m_data;
};

}    // namespace sw::containers::spsc
