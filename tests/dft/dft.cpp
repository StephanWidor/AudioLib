#include <sw/chrono/stopwatch.hpp>
#include <sw/dft/transform.hpp>
#include <sw/signals.hpp>

#include <gtest/gtest.h>

#include <algorithm>
#include <list>

namespace {

template<std::floating_point F>
bool spectrumIsReal(const std::vector<std::complex<F>> &spectrum, const F tolerance = sw::math::defaultTolerance<F>)
{
    const auto N = spectrum.size();
    const auto n = N / 2;
    auto im = spectrum[0].imag();
    if (std::abs(im) > tolerance)
        return false;
    im = spectrum[n].imag();
    if (std::abs(im) > tolerance)
        return false;
    for (auto k = 1u; k < n; ++k)
    {
        const auto &c0 = spectrum[k];
        const auto &c1 = spectrum[N - k];
        if (!sw::math::isZero(c0 - std::conj(c1), tolerance))
            return false;
    }
    return true;
}

}    // namespace

namespace sw::dft::tests {

template<std::floating_point F>
class DFTTest : public ::testing::Test
{};
using FloatTypes = ::testing::Types<float, double>;
TYPED_TEST_SUITE(DFTTest, FloatTypes);

TYPED_TEST(DFTTest, RealVsComplexValues)
{
    using F = TypeParam;

    auto check = [&](const auto &signal, auto &processor) {
        const auto signalComplex = signal | std::ranges::views::transform([](const auto s) { return std::complex(s); });
        std::vector<std::complex<F>> spectrumReal(signal.size()), spectrumComplex(signal.size());

        processor.transform(signalComplex, spectrumComplex);
        processor.transform(signal, spectrumReal);

        EXPECT_TRUE(equal<std::complex<F>>(spectrumReal, spectrumComplex));
        EXPECT_TRUE(spectrumIsReal(spectrumReal));
        EXPECT_TRUE(spectrumIsReal(spectrumComplex));

        std::vector<std::complex<F>> signalInv(spectrumReal.size());
        processor.transform_inverse(spectrumReal, signalInv);
        EXPECT_TRUE(equal<std::complex<F>>(signalComplex, signalInv));

        processor.transform_inverse(spectrumComplex, signalInv);
        EXPECT_TRUE(equal<std::complex<F>>(signalComplex, signalInv));
    };

    for (auto N = 2u; N <= 256; N *= 2u)
    {
        const auto signal = makeRandomRealSignal(math::one<F>, N, N);
        const std::list<F> signalAsList(signal.begin(), signal.end());

        DFT<F> dft(N);
        check(signal, dft);
        check(signalAsList, dft);

        FFT<F> fft(N);
        check(signal, fft);
        check(signalAsList, fft);
    }
}

TYPED_TEST(DFTTest, OnlyFirstHalf)
{
    using F = TypeParam;

    auto check = [&](const std::vector<F> &signal, auto &processor) {
        const auto N = signal.size();
        const auto n = N / 2u + 1u;
        std::vector<std::complex<F>> spectrum(N), spectrumShort(n);
        processor.transform(signal, spectrum);
        processor.transform(signal, spectrumShort);
        EXPECT_TRUE(std::equal(spectrumShort.begin(), spectrumShort.end(), spectrum.begin(),
                               [&](const auto &v0, const auto &v1) { return sw::math::equal(v0, v1); }));
    };

    for (auto N = 2u; N <= 256; N *= 2u)
    {
        const auto signal = makeRandomRealSignal(math::one<F>, N, N);

        DFT<F> dft(signal.size());
        check(signal, dft);

        FFT<F> fft(signal.size());
        check(signal, fft);
    }
}

TYPED_TEST(DFTTest, CrossValidate)
{
    using F = TypeParam;

    auto crossValidate = [&](const std::vector<std::complex<F>> &signal) {
        const auto N = signal.size();
        DFT<F> dft(N);
        FFT<F> fft(N);
        FFT<F> fft_dac(N);
        std::vector<std::complex<F>> spectrumDFT(N), spectrumFFT(N), spectrumFFT_dac(N);

        dft.transform(signal, spectrumDFT);
        fft.transform(signal, spectrumFFT);
        fft_dac.transform(signal, spectrumFFT_dac);
        EXPECT_TRUE(equal<std::complex<F>>(spectrumDFT, spectrumFFT));
        EXPECT_TRUE(equal<std::complex<F>>(spectrumFFT, spectrumFFT_dac));

        std::vector<std::complex<F>> signalInv(N);
        dft.transform_inverse(spectrumFFT, signalInv);
        EXPECT_TRUE(equal<std::complex<F>>(signal, signalInv));

        fft.transform_inverse(spectrumDFT, signalInv);
        EXPECT_TRUE(equal<std::complex<F>>(signal, signalInv));

        fft_dac.transform_inverse(spectrumFFT_dac, signalInv);
        EXPECT_TRUE(equal<std::complex<F>>(signal, signalInv));
    };

    const auto maxN = 1024;
    for (auto N = 2u; N <= maxN; N *= 2u)
        crossValidate(makeRandomComplexSignal(math::one<F>, N, N));
}

TYPED_TEST(DFTTest, TransformDirac)
{
    using F = TypeParam;

    const auto signal = makeDirac(math::one<F>, 2048u);

    std::vector<std::complex<F>> spectrum(signal.size());
    FFT<F>(signal.size()).transform(signal, spectrum);
    EXPECT_TRUE(std::all_of(
      spectrum.begin(), spectrum.end(),
      [oneComplex = std::complex{math::one<F>, math::zero<F>}](const auto c) { return math::equal(c, oneComplex); }));
}

TYPED_TEST(DFTTest, TransformDirectCurrent)
{
    using F = TypeParam;

    const auto signalLength = 2048u;
    const std::vector<F> signal(signalLength, math::one<F>);
    std::vector<std::complex<F>> spectrum(signal.size());
    FFT<F>(signal.size()).transform(signal, spectrum);
    EXPECT_TRUE(math::equal(spectrum.front(), std::complex{static_cast<F>(signalLength), math::zero<F>}));
    EXPECT_TRUE(std::all_of(spectrum.begin() + 1, spectrum.end(),
                            [zeroComplex = std::complex{math::zero<F>, math::zero<F>}](const auto c) {
                                return math::equal(c, zeroComplex);
                            }));
}

TYPED_TEST(DFTTest, SingleSineWaves)
{
    using F = TypeParam;

    constexpr size_t N = 32;
    constexpr auto n = N / 2;
    constexpr auto sampleRate = static_cast<F>(48000);
    FFT<F> fft(N);
    auto frequencies = dft::makeBinFrequencies(N, sampleRate, n);
    for (auto i = 1u; i < frequencies.size(); ++i)
    {
        const auto signal = makeSineWave(math::one<F>, frequencies[i], sampleRate, N);
        std::vector<std::complex<F>> spectrum(N);
        fft.transform(signal, spectrum);
        for (auto j = 1u; j < frequencies.size(); ++j)
        {
            if (j == i)
            {
                EXPECT_NEAR(spectrum[j].imag(), -static_cast<F>(n), math::defaultTolerance<F>);
            }
            EXPECT_NEAR(std::abs(spectrum[j] + spectrum[N - j]), math::zero<F>, math::defaultTolerance<F>);
        }
    }
}

}    // namespace sw::dft::tests
