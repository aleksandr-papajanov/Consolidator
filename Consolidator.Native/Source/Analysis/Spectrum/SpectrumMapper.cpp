#include "Analysis/Spectrum/SpectrumMapper.h"

#include <algorithm>
#include <cmath>

namespace consolidator::analysis
{

namespace
{

constexpr double kMinimumFrequencyHz = 20.0;
constexpr double kMaximumFrequencyHz = 20000.0;
constexpr float kMinimumMagnitude = 1.0e-12F;
constexpr float kMinimumMagnitudeDb = -240.0F;

} // namespace

void SpectrumMapper::Calculate(
    const RawSpectrum& input,
    SpectrumSnapshot& output) const noexcept
{
    output.revision = input.revision;
    output.sampleRate = input.sampleRate;

    if (input.sampleRate <= 0.0)
    {
        output.magnitudeDb.fill(kMinimumMagnitudeDb);
        return;
    }

    const auto maximumFrequency = std::min(
        kMaximumFrequencyHz,
        input.sampleRate * 0.5);
    if (maximumFrequency <= kMinimumFrequencyHz)
    {
        output.magnitudeDb.fill(kMinimumMagnitudeDb);
        return;
    }

    const auto logMinimum = std::log(kMinimumFrequencyHz);
    const auto logMaximum = std::log(maximumFrequency);
    for (std::size_t index = 0; index < kDisplaySpectrumBinCount; ++index)
    {
        const auto position = static_cast<double>(index) /
            static_cast<double>(kDisplaySpectrumBinCount - 1);
        const auto frequency = std::exp(
            logMinimum + position * (logMaximum - logMinimum));
        const auto rawBin = frequency * static_cast<double>(kFftSize) /
            input.sampleRate;
        const auto lowerBin = static_cast<std::size_t>(rawBin);
        const auto clampedLowerBin = std::min(
            lowerBin, kSpectrumBinCount - 1);
        const auto upperBin = std::min(
            clampedLowerBin + 1, kSpectrumBinCount - 1);
        const auto fraction = static_cast<float>(
            rawBin - static_cast<double>(clampedLowerBin));
        const auto magnitude = input.magnitudes[clampedLowerBin] +
            fraction * (input.magnitudes[upperBin] -
                        input.magnitudes[clampedLowerBin]);
        output.magnitudeDb[index] = 20.0F * std::log10(std::max(
            magnitude, kMinimumMagnitude));
    }
}

} // namespace consolidator::analysis
