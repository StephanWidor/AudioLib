#pragma once
#include "sw/dft/utils.hpp"

#include <tbb/parallel_for.h>

#include <cassert>
#include <concepts>
#include <span>
#include <vector>

namespace sw::dft {

enum class Algorithm
{
    LinearTransform,
    FFT
};

namespace detail {

template<std::floating_point F>
std::vector<std::complex<F>> unitRoots(const size_t N)
{
    std::vector<std::complex<F>> roots(N);
    const auto initRoots = [&](const auto numInit) {
        const auto angle = sw::math::twoPi<F> / static_cast<F>(N);
        for (auto i = 0u; i < numInit; ++i)
            roots[i] = std::polar(static_cast<F>(1.0), static_cast<F>(i) * angle);
    };

    // we can use unit circle symmetry and rotate instead of expensive polar for all roots
    if (N % 2u == 0)
    {
        const auto halfN = static_cast<int>(N) / 2;
        if (N % 4u == 0)
        {
            const auto quarterN = halfN / 2;
            initRoots(static_cast<size_t>(quarterN));
            std::transform(roots.begin(), roots.begin() + quarterN, roots.begin() + quarterN,
                           [](const auto &c) { return std::complex<F>{-c.imag(), c.real()}; });
        }
        else
            initRoots(static_cast<size_t>(halfN));

        std::transform(roots.begin(), roots.begin() + halfN, roots.begin() + halfN,
                       [](const auto &c) { return std::complex{-c.real(), -c.imag()}; });
    }
    else
        initRoots(N);
    return roots;
}

template<std::floating_point F, sw::dft::Algorithm>
struct Processor;

template<std::floating_point F>
struct Processor<F, Algorithm::LinearTransform>
{
    template<bool doParallel, std::invocable<size_t> UnitRootGetter>
    static void run(std::vector<std::complex<F>> &inSwap, std::vector<std::complex<F>> &outSwap,
                    const UnitRootGetter &unitRoot)
    {
        const auto N = std::ranges::ssize(inSwap);
        assert(N == std::ranges::ssize(outSwap));

        const auto processOutVal = [&](const auto k) {
            auto &outVal = outSwap[k] = math::zero<F>;
            for (auto j = 0; j < N; ++j)
                outVal += (inSwap[j] * unitRoot(j * k));
        };

        if constexpr (doParallel)
        {
            tbb::parallel_for(tbb::blocked_range<size_t>(0, N), [&](const tbb::blocked_range<size_t> &r) {
                for (auto k = r.begin(); k != r.end(); ++k)
                    processOutVal(k);
            });
        }
        else
        {
            for (auto k = 0u; k < N; ++k)
                processOutVal(k);
        }
    }
};

template<std::floating_point F>
struct Processor<F, Algorithm::FFT>
{
    template<bool doParallel, std::invocable<size_t> UnitRootGetter>
    static void run(std::vector<std::complex<F>> &inSwap, std::vector<std::complex<F>> &outSwap,
                    const UnitRootGetter &unitRoot)
    {
        const auto N = std::ranges::ssize(inSwap);
        assert(N == std::ranges::ssize(outSwap));
        const auto n = N / 2;

        for (auto numPartitions = 1; numPartitions < N; numPartitions *= 2)
        {
            const auto processPartition = [&](const auto k) {
                const auto partitionSize = N / (numPartitions * 2);
                const auto startJ = partitionSize * k;
                const auto index0 = startJ * 2u;
                const auto index1 = index0 + partitionSize;
                const auto &root = unitRoot(startJ);
                for (auto shift = 0; shift < partitionSize; ++shift)
                {
                    const auto &rootProduct = root * inSwap[index1 + shift];
                    const auto &out0 = inSwap[index0 + shift];
                    const auto j = startJ + shift;
                    outSwap[j] = out0 + rootProduct;
                    outSwap[j + n] = out0 - rootProduct;
                }
            };

            if constexpr (doParallel)
            {
                tbb::parallel_for(tbb::blocked_range<int>(0, static_cast<int>(numPartitions)),
                                  [&](const tbb::blocked_range<int> &r) {
                                      for (auto k = r.begin(); k != r.end(); ++k)
                                          processPartition(k);
                                  });
            }
            else
            {
                for (auto k = 0; k < numPartitions; ++k)
                    processPartition(k);
            }

            inSwap.swap(outSwap);
        }
        inSwap.swap(outSwap);
    }
};

}    // namespace detail

template<std::floating_point F, bool doParallel, Algorithm Algorithm>
class Transform
{
public:
    static constexpr bool doingParallel = doParallel;

    Transform(const size_t signalLength)
        : m_unitRoots(detail::unitRoots<F>(signalLength)), m_inSwap(signalLength), m_outSwap(signalLength)
    {
        if constexpr (Algorithm == Algorithm::FFT)
            assert(sw::math::isPowerOfTwo(signalLength));
        else
            assert(signalLength % 2 == 0u);
    }

    size_t signalLength() const { return m_unitRoots.size(); }

    size_t nyquistLength() const { return sw::dft::nyquistLength(signalLength()); }

    template<ranges::TypedInputRange<std::complex<F>> ComplexInRange,
             ranges::TypedOutputRange<std::complex<F>> ComplexOutRange>
    void transform(ComplexInRange &&complexSignal, ComplexOutRange &&o_coefficients)
    {
        assert(static_cast<size_t>(std::ranges::size(complexSignal)) == signalLength());
        m_inSwap.resize(signalLength());
        m_outSwap.resize(signalLength());
        std::ranges::copy(complexSignal, m_inSwap.begin());

        detail::Processor<F, Algorithm>::template run<doingParallel>(m_inSwap, m_outSwap, unitRootGetter_Negative());
        std::ranges::copy(m_outSwap, o_coefficients.begin());
    }

    template<ranges::TypedInputRange<std::complex<F>> ComplexInRange,
             ranges::TypedOutputRange<std::complex<F>> ComplexOutRange>
    void transform_inverse(ComplexInRange &&coefficients, ComplexOutRange &&o_complexSignal)
    {
        assert(static_cast<size_t>(std::ranges::size(coefficients)) == signalLength());
        m_inSwap.resize(signalLength());
        m_outSwap.resize(signalLength());
        std::ranges::copy(coefficients, m_inSwap.begin());

        detail::Processor<F, Algorithm>::template run<doingParallel>(m_inSwap, m_outSwap, unitRootGetter_Positive());
        std::ranges::transform(
          m_outSwap, o_complexSignal.begin(),
          [factor = math::one<F> / static_cast<F>(signalLength())](const auto &s) { return s * factor; });
    }

    template<ranges::TypedInputRange<F> RealInRange, ranges::TypedOutputRange<std::complex<F>> ComplexOutRange>
    void transform(RealInRange &&realSignal, ComplexOutRange &&o_coefficients)
    {
        assert(static_cast<size_t>(std::ranges::size(realSignal)) == signalLength());
        const auto outSize = std::ranges::ssize(o_coefficients);
        assert(outSize == static_cast<int>(signalLength()) || outSize == static_cast<int>(nyquistLength()));

        const auto n = signalLength() / 2;
        m_inSwap.resize(n);
        m_outSwap.resize(n);

        auto inIt = realSignal.begin();
        for (auto i = 0u; i < n; ++i)
        {
            auto &c = m_inSwap[i];
            c.real(*(inIt++));
            c.imag(*(inIt++));
        }

        const auto unitRoot = unitRootGetter_Negative();
        const auto unitRoot_n = [&](const auto i) { return unitRoot(2 * i); };
        detail::Processor<F, Algorithm>::template run<doingParallel>(m_inSwap, m_outSwap, unitRoot_n);

        m_outSwap.resize(n + 1);
        m_outSwap[n] = {m_outSwap[0].real() - m_outSwap[0].imag(), math::zero<F>};
        m_outSwap[0] = {m_outSwap[0].real() + m_outSwap[0].imag(), math::zero<F>};

        const auto evenAndOdd = [&](const auto k) {
            return std::make_pair(math::oneHalf<F> * (m_outSwap[k] + std::conj(m_outSwap[n - k])),
                                  math::oneHalf<F> * std::complex<F>{m_outSwap[k].imag() + m_outSwap[n - k].imag(),
                                                                     m_outSwap[n - k].real() - m_outSwap[k].real()});
        };

        const auto deflateComplex = [&](const auto k) {
            const auto [even_k, odd_k] = evenAndOdd(k);
            const auto [even_nMk, odd_nMk] = evenAndOdd(n - k);
            m_outSwap[k] = even_k + unitRoot(k) * odd_k;
            m_outSwap[n - k] = even_nMk + unitRoot(n - k) * odd_nMk;
        };

        const auto nHalf = n / 2;
        if constexpr (doingParallel)
        {
            tbb::parallel_for(tbb::blocked_range<int>(1, static_cast<int>(nHalf + 1)),
                              [&](const tbb::blocked_range<int> &r) {
                                  for (auto k = r.begin(); k != r.end(); ++k)
                                      deflateComplex(k);
                              });
        }
        else
        {
            for (auto k = 1; k <= nHalf; ++k)
                deflateComplex(k);
        }

        if (static_cast<size_t>(outSize) == signalLength())
        {
            m_outSwap.resize(signalLength());
            makeSecondHalfConjugate(m_outSwap);
        }

        std::ranges::copy(m_outSwap, o_coefficients.begin());
    }

    /// not quite optimized...
    template<ranges::TypedInputRange<std::complex<F>> ComplexInRange, ranges::TypedOutputRange<F> RealOutRange>
    void transform_inverse(ComplexInRange &&coefficients, RealOutRange &&o_realSignal)
    {
        const auto inSize = static_cast<size_t>(std::ranges::size(coefficients));
        assert(inSize == signalLength() || inSize == nyquistLength());
        assert(static_cast<size_t>(std::ranges::size(o_realSignal)) == signalLength());

        m_inSwap.resize(signalLength());
        m_outSwap.resize(signalLength());
        std::ranges::copy(coefficients, m_inSwap.begin());
        assert(math::isZero(m_inSwap[0].imag()));
        assert(math::isZero(m_inSwap[nyquistLength() - 1u].imag()));
        if (inSize == nyquistLength())
            makeSecondHalfConjugate(m_inSwap);

        detail::Processor<F, Algorithm>::template run<doingParallel>(m_inSwap, m_outSwap, unitRootGetter_Positive());
        std::ranges::transform(
          m_outSwap, o_realSignal.begin(),
          [factor = math::one<F> / static_cast<F>(signalLength())](auto &s) { return std::real(s) * factor; });
    }

private:
    auto unitRootGetter_Positive() const
    {
        return [this, N = m_unitRoots.size()](auto i) { return m_unitRoots[i % N]; };
    }

    auto unitRootGetter_Negative() const
    {
        return [this, N = m_unitRoots.size()](auto i) { return m_unitRoots[(i * N - i) % N]; };
    }

    static void makeSecondHalfConjugate(const std::span<std::complex<F>> io_coefficients)
    {
        const auto N = std::ranges::ssize(io_coefficients);
        assert(math::isZero(io_coefficients.front().imag()));
        assert(N <= 2 || math::isZero(io_coefficients[N / 2].imag()));
        std::transform(io_coefficients.begin() + 1, io_coefficients.begin() + (N / 2), io_coefficients.rbegin(),
                       [](const auto &s) { return std::conj(s); });
    }

    std::vector<std::complex<F>> m_unitRoots;
    std::vector<std::complex<F>> m_inSwap, m_outSwap;
};

template<std::floating_point F, bool doParallel = false>
using DFT = Transform<F, doParallel, Algorithm::LinearTransform>;

template<std::floating_point F, bool doParallel = false>
using FFT = Transform<F, doParallel, Algorithm::FFT>;

}    // namespace sw::dft
