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
    static void Add(
        DspAudioBuffer& destination,
        const DspAudioBuffer& processed,
        DspChannelMode channelMode) noexcept
    {
        const auto processLeft = channelMode != DspChannelMode::Right;
        const auto processRight = channelMode != DspChannelMode::Left;
        for (std::size_t index = 0; index < destination.frameCount; ++index)
        {
            if (processLeft &&
                destination.left != nullptr &&
                processed.left != nullptr)
            {
                destination.left[index] += processed.left[index];
            }
            if (processRight &&
                destination.right != nullptr &&
                processed.right != nullptr)
            {
                destination.right[index] += processed.right[index];
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
