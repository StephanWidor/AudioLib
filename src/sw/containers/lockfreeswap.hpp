#pragma once
#include "sw/containers/utils.hpp"

#include <atomic>
#include <optional>

namespace sw::containers {

template<std::copyable ValueType>
class LockFreeSwap
{
public:
    using Value = ValueType;

    LockFreeSwap(const Value &initValue): m_data{makeFilledArray<3>(initValue)} {}

    // feeding side

    Value &inSwap()
    {
        if (!m_writingPage.has_value())
            m_writingPage = nextWritingPage();
        return m_data[*m_writingPage];
    }

    void set(const Value &value) { inSwap() = value; }

    void push()
    {
        if (m_writingPage.has_value())
        {
            m_writtenPage.store(*m_writingPage, std::memory_order::release);
            m_writingPage.reset();
        }
    }

    // consumer side

    const Value &pull() const
    {
        m_readingPage.store(m_writtenPage.load(std::memory_order::acquire), std::memory_order::release);
        return m_data[m_readingPage.load(std::memory_order::relaxed)];
    }

private:
    size_t nextWritingPage() const
    {
        size_t writingPage = (m_writtenPage.load(std::memory_order::relaxed) + 1) % 3;
        if (writingPage == m_readingPage.load(std::memory_order::acquire))
            writingPage = (writingPage + 1) % 3;
        return writingPage;
    }

    std::array<Value, 3> m_data;
    mutable std::atomic<size_t> m_readingPage{0u};
    std::atomic<size_t> m_writtenPage{0u};
    std::optional<size_t> m_writingPage{};
};

}    // namespace sw::containers
