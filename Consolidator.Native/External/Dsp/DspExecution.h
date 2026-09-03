#pragma once

#include <cstddef>

#include "DspProcessingContext.h"
#include "SharedDspState.h"

namespace consolidator::max::dsp
{

struct DspExecutionPolicy
{
    bool audible{};
    bool instanceBypassed{};
    bool inputActive{};
    bool levelerEnabled{};
    bool saturatorActive{};
    bool saturatorSplit{};
    bool compressorActive{};
    bool compressorParallel{};
    bool equalizerActive{};
    bool polishActive{};
    bool outputActive{};
    bool limiterEnabled{};
    bool widthProcessingRequired{};
    DspChannelMode channelMode{ DspChannelMode::Stereo };
};

struct DspProcessingFrame
{
    DspAudioBuffer main{};
    DspSidechainBuffer externalSidechain{};
    DspSidechainBuffer detectorSidechain{};
    const DspProcessingPlan& plan;
    const DspExecutionPolicy& policy;
};

[[nodiscard]] DspExecutionPolicy BuildExecutionPolicy(
    const DspSnapshot& snapshot,
    const DspProcessingPlan& plan) noexcept;

} // namespace consolidator::max::dsp
