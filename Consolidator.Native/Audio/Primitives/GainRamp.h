#pragma once

#include "ParameterRamp.h"
#include "../Core/AudioBuffer.h"

namespace consolidator::audio::primitives
{

class GainRamp final
{
public:
    void Prepare(const ProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetTargetDb(float targetDb) noexcept;
    void Process(
        AudioBuffer& audio,
        ChannelMode channelMode) noexcept;

    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    ParameterRamp gain_{ 1.0 };
};

} // namespace consolidator::audio::primitives
