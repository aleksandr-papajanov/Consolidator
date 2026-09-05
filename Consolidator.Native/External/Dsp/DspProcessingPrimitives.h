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
