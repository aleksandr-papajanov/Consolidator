#pragma once

#include <cstddef>

#include "DspExecution.h"

namespace consolidator::max::dsp
{

struct DspChannelView
{
    double* left{};
    double* right{};
    std::size_t channelCount{};
};

[[nodiscard]] inline DspChannelView SelectChannels(
    DspAudioBuffer& audio,
    DspChannelMode mode) noexcept
{
    switch (mode)
    {
        case DspChannelMode::Left:
            return { audio.left, nullptr, 1 };
        case DspChannelMode::Right:
            return { nullptr, audio.right, 1 };
        case DspChannelMode::Stereo:
        default:
            return { audio.left, audio.right, 2 };
    }
}

class DspParallelMixer final
{
public:
    static void MixEqual(
        DspAudioBuffer& destination,
        const DspAudioBuffer& processed) noexcept
    {
        for (std::size_t index = 0; index < destination.frameCount; ++index)
        {
            if (destination.left != nullptr && processed.left != nullptr)
            {
                destination.left[index] =
                    (destination.left[index] + processed.left[index]) * 0.5;
            }
            if (destination.right != nullptr && processed.right != nullptr)
            {
                destination.right[index] =
                    (destination.right[index] + processed.right[index]) * 0.5;
            }
        }
    }
};

class DspFrequencyBandRunner final
{
public:
    template <typename ProcessBand>
    static void Run(
        const DspFrequencyBandSet& bands,
        ProcessBand&& processBand) noexcept
    {
        for (std::size_t index = 0; index < bands.count; ++index)
        {
            processBand(bands.ranges[index]);
        }
    }
};

} // namespace consolidator::max::dsp
