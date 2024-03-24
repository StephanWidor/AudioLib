#pragma once
#include "sw/dft/utils.hpp"
#include "sw/phases.hpp"
#include "sw/signals.hpp"
#include "sw/spectrum.hpp"

namespace sw::dft {

/// Use phase of dft coefficient to "correct" bin frequency
template<std::floating_point F>
F correctedFrequency(const F lastPhase, const F coefficientPhase, const F timeDiff, const F binFrequency)
{
    const auto expectedAngle = phaseAngle(binFrequency, timeDiff);
    const auto expectedPhase = math::angles::standardized(lastPhase + expectedAngle);
    const auto phaseDiff = math::angles::standardized(coefficientPhase - expectedPhase);
    const auto angle = expectedAngle + phaseDiff;
    const auto frequency = sw::frequency(angle, timeDiff);
    return std::abs(frequency);
}

template<std::floating_point F>
void toSpectrum(const F sampleRate, ranges::TypedInputRange<std::complex<F>> auto &&coefficients,
                ranges::TypedOutputRange<SpectrumValue<F>> auto &&o_spectrum)
{
    const auto numCoefficients = std::ranges::ssize(coefficients);
    assert(numCoefficients == std::ranges::size(o_spectrum));

    const auto binFrequencies = dft::binFrequencies(numCoefficients, sampleRate);

    for (auto i = 0u; i < numCoefficients; ++i)
    {
        auto &spectrumValue = o_spectrum[i];
        spectrumValue.frequency = binFrequencies[i];
        spectrumValue.gain = std::abs(coefficients[i]);
    }
}

/// Use phases of dft coefficients to "correct" bin frequencies
/// lastBinPhases and o_binPhases can be the same range, to update phases in place
template<std::floating_point F, ranges::TypedOutputRange<SpectrumValue<F>> SpectrumRange>
void toSpectrumByPhase(const F sampleRate, const F timeDiff, ranges::TypedInputRange<F> auto &&lastBinPhases,
                       ranges::TypedInputRange<std::complex<F>> auto &&binCoefficients, SpectrumRange &&o_binSpectrum,
                       ranges::TypedOutputRange<F> auto &&o_binPhases)
{
    assert(std::ranges::size(binCoefficients) > 1u &&
           std::ranges::size(binCoefficients) == std::ranges::size(lastBinPhases) &&
           std::ranges::size(binCoefficients) == std::ranges::size(o_binSpectrum) &&
           std::ranges::size(binCoefficients) == std::ranges::size(o_binPhases));

    const auto halfSignalLength = std::ranges::size(binCoefficients) - 1u;
    const auto signalLength = 2u * halfSignalLength;

    const auto o_gains = gains<F>(std::forward<SpectrumRange>(o_binSpectrum));
    const auto o_frequencies = frequencies<F>(std::forward<SpectrumRange>(o_binSpectrum));

    std::ranges::copy(dft::binFrequencies(signalLength, sampleRate) |
                        std::views::take(std::ranges::ssize(o_binSpectrum)),
                      o_frequencies.begin());

    // we abuse spectrum gains to carry new phases temporarily (correctedFrequency needs last and new phases, and as
    // we offer the possibility to let lastBinPhases and o_binPhases be the same range, we can't write directly into
    // o_binPhases)
    std::ranges::transform(binCoefficients, o_gains.begin(), [](const auto &c) { return std::arg(c); });

    std::ranges::transform(
      lastBinPhases, o_binSpectrum, o_frequencies.begin(), [&](const auto lastPhase, const auto &binFrequencyAndPhase) {
          return correctedFrequency(lastPhase, binFrequencyAndPhase.gain, timeDiff, binFrequencyAndPhase.frequency);
      });
    std::ranges::copy(o_gains, o_binPhases.begin());

    std::ranges::transform(binCoefficients, o_gains.begin(),
                           [gainFactor = math::one<F> / static_cast<F>(halfSignalLength)](const auto &c) {
                               return gainFactor * std::abs(c);
                           });
}

}    // namespace sw::dft
