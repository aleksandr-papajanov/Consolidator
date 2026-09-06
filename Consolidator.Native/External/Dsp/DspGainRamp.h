#pragma once

#include "DspParameterRamp.h"
#include "DspProcessingContext.h"

namespace consolidator::max::dsp
{

class DspGainRamp final
{
public:
    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetTargetDb(float targetDb) noexcept;
    void Process(
        DspAudioBuffer& audio,
        DspChannelMode channelMode) noexcept;

    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    DspParameterRamp gain_{ 1.0 };
};

} // namespace consolidator::max::dsp
