#pragma once
#include "sw/containers/spinlockedringbuffer.hpp"
#include "sw/math/math.hpp"
#include "sw/ranges/defines.hpp"

#include <cassert>

namespace sw {

template<std::floating_point F>
class ProcessingBuffer
{
public:
    ProcessingBuffer(const size_t signalBufferSize, const size_t stepSize)
        : m_inputBuffer(signalBufferSize, math::zero<F>)
        , m_outputBuffer(signalBufferSize, math::zero<F>)
        , m_processedSignalBuffer(stepSize, math::zero<F>)
    {
        assert(signalBufferSize >= stepSize);
    }

    void process(ranges::TypedInputRange<F> auto &&inSignal, ranges::TypedOutputRange<F> auto &&outSignal,
                 const std::invocable<const std::span<const F>, std::vector<F> &> auto &stepProcessCallback)
    {
        const auto numSamples = static_cast<size_t>(std::ranges::size(inSignal));
        assert(numSamples == static_cast<size_t>(std::ranges::size(outSignal)));
        assert(numSamples <= m_inputBuffer.inBuffer().size());
        const auto stepSize = m_processedSignalBuffer.size();

        m_inputBuffer.push(inSignal);

        for (m_numNewProcessingSamples += numSamples; m_numNewProcessingSamples >= stepSize;
             m_numNewProcessingSamples -= stepSize)
        {
            const auto stepSignal =
              std::span(m_inputBuffer.inBuffer().end() - static_cast<int>(m_numNewProcessingSamples), stepSize);

            stepProcessCallback(stepSignal, m_processedSignalBuffer);

            m_outputBuffer.push(m_processedSignalBuffer);
            m_numOutSamples += stepSize;
        }

        if (m_numOutSamples < numSamples)
            m_numOutSamples = numSamples;

        assert(m_numOutSamples <= m_outputBuffer.inBuffer().size());
        const auto outStart = m_outputBuffer.inBuffer().end() - static_cast<int>(m_numOutSamples);
        std::copy(outStart, outStart + numSamples, outSignal.begin());

        assert(m_numOutSamples >= numSamples);
        m_numOutSamples -= numSamples;
    }

    const std::vector<F> &inputBuffer() const { return m_inputBuffer.outBuffer(); }
    const std::vector<F> &outputBuffer() const { return m_outputBuffer.outBuffer(); }

private:
    containers::SpinLockedRingBuffer<F> m_inputBuffer;
    containers::SpinLockedRingBuffer<F> m_outputBuffer;
    std::vector<F> m_processedSignalBuffer;

    size_t m_numNewProcessingSamples{0u};
    size_t m_numOutSamples{0u};
};

}    // namespace sw
