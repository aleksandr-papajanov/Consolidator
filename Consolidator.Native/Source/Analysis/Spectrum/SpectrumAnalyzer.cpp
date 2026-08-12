#include "Analysis/Spectrum/SpectrumAnalyzer.h"

#include <cmath>
#include <exception>

namespace consolidator::analysis
{

namespace
{

constexpr float kPi = 3.14159265358979323846F;

} // namespace

SpectrumAnalyzer::SpectrumAnalyzer()
    : fftConfig_(kiss_fftr_alloc(static_cast<int>(kFftSize), 0, nullptr, nullptr))
{
    if (!fftConfig_)
    {
        std::terminate();
    }

    for (std::size_t index = 0; index < kFftSize; ++index)
    {
        windowFunction_[index] = 0.5F - 0.5F * std::cos(
            2.0F * kPi * static_cast<float>(index) /
            static_cast<float>(kFftSize - 1));
        windowSum_ += windowFunction_[index];
    }
}

void SpectrumAnalyzer::FftConfigDeleter::operator()(
    kiss_fftr_state* config) const noexcept
{
    if (config != nullptr)
    {
        kiss_fftr_free(config);
    }
}

void SpectrumAnalyzer::Calculate(
    const AudioWindow& input,
    RawSpectrum& output) noexcept
{
    output.revision = input.revision;
    output.sampleRate = input.sampleRate;
    for (std::size_t index = 0; index < kFftSize; ++index)
    {
        fftInput_[index] = input.samples[index] * windowFunction_[index];
    }

    kiss_fftr(fftConfig_.get(), fftInput_.data(), fftOutput_.data());
    for (std::size_t index = 0; index < kSpectrumBinCount; ++index)
    {
        const auto amplitude = std::hypot(
            fftOutput_[index].r,
            fftOutput_[index].i) / windowSum_;
        const auto isOneSidedInteriorBin =
            index != 0 && index != kFftSize / 2;
        output.magnitudes[index] = isOneSidedInteriorBin
            ? 2.0F * amplitude
            : amplitude;
    }
}

} // namespace consolidator::analysis
