#pragma once

#include <array>
#include <cstddef>
#include <cmath>

namespace consolidator::max::dsp
{

enum class DspChannelMode
{
    Stereo,
    Left,
    Right
};

struct DspFrequencyRange
{
    double minimumHz{};
    double maximumHz{};
};

struct DspFrequencyBandSet
{
    static constexpr std::size_t kMaximumBandCount = 8;

    std::array<DspFrequencyRange, kMaximumBandCount> ranges{};
    std::size_t count{};

    void Clear() noexcept
    {
        count = 0;
    }

    [[nodiscard]] bool Add(
        double minimumHz,
        double maximumHz) noexcept
    {
        if (count >= kMaximumBandCount ||
            !std::isfinite(minimumHz) ||
            !std::isfinite(maximumHz) ||
            minimumHz < 0.0 ||
            maximumHz <= minimumHz)
        {
            return false;
        }

        ranges[count++] = { minimumHz, maximumHz };
        return true;
    }

    [[nodiscard]] bool IsValid(double sampleRate) const noexcept
    {
        if (!std::isfinite(sampleRate) || sampleRate <= 0.0)
        {
            return false;
        }

        const auto nyquist = sampleRate * 0.5;
        if (!std::isfinite(nyquist) || count > kMaximumBandCount)
        {
            return false;
        }

        for (std::size_t index = 0; index < count; ++index)
        {
            const auto& range = ranges[index];
            if (!std::isfinite(range.minimumHz) ||
                !std::isfinite(range.maximumHz) ||
                range.minimumHz < 0.0 ||
                range.maximumHz <= range.minimumHz ||
                range.maximumHz > nyquist)
            {
                return false;
            }
        }

        return true;
    }
};

struct DspProcessingPlan
{
    DspChannelMode channelMode{ DspChannelMode::Stereo };
    DspFrequencyBandSet frequencyBands{};
};

struct DspProcessingContext
{
    static constexpr std::size_t kDefaultMaximumFrameCount = 4096;

    double sampleRate{};
    std::size_t maximumFrameCount{};
};

struct DspAudioBuffer
{
    double* left{};
    double* right{};
    std::size_t frameCount{};
};

struct DspSidechainBuffer
{
    const double* left{};
    const double* right{};
};

} // namespace consolidator::max::dsp
