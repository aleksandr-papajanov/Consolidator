#pragma once

#include <array>
#include <memory>

#include "Analysis/Spectrum/SpectrumTypes.h"
#include "Analysis/KissFFT/kiss_fftr.h"

namespace consolidator::analysis
{

// Converts one audio window into FFT magnitudes using one reusable FFT config.
class SpectrumAnalyzer final
{
public:
    SpectrumAnalyzer();

    void Calculate(
        const AudioWindow& input,
        RawSpectrum& output) noexcept;

    SpectrumAnalyzer(const SpectrumAnalyzer&) = delete;
    SpectrumAnalyzer& operator=(const SpectrumAnalyzer&) = delete;

private:
    struct FftConfigDeleter
    {
        void operator()(kiss_fftr_state* config) const noexcept;
    };

    std::unique_ptr<kiss_fftr_state, FftConfigDeleter> fftConfig_;
    std::array<float, kFftSize> windowFunction_{};
    float windowSum_ = 0.0F;
    std::array<kiss_fft_scalar, kFftSize> fftInput_{};
    std::array<kiss_fft_cpx, kSpectrumBinCount> fftOutput_{};
    std::array<float, kSpectrumBinCount> leftMagnitudes_{};
};

} // namespace consolidator::analysis
