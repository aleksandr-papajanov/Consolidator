#pragma once

#include "ParameterRamp.h"

namespace consolidator::audio::primitives
{

class Width final
{
public:
    void Prepare(const ProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetAmount(double width) noexcept;
    void Process(AudioBuffer audio, ChannelMode mode) noexcept;

private:
    ParameterRamp amount_{ 1.0 };
};

} // namespace consolidator::audio::primitives
