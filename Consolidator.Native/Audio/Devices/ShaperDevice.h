#pragma once

#include "WetTransition.h"
#include "../Algorithms/TransientShaper/TransientShaperProcessor.h"

namespace consolidator::audio::devices
{

struct ShaperParameters
{
    double attack{};
    double sustain{};
    double outputDb{};
    bool bypass{};
};

class ShaperDevice final
{
public:
    void Prepare(const ProcessingContext& context);
    void Reset() noexcept;
    void SetParameters(const ShaperParameters& parameters) noexcept;
    void Process(AudioBuffer audio, ChannelMode mode) noexcept;

private:
    algorithms::TransientShaperProcessor shaper_;
    WetTransition transition_;
};

} // namespace consolidator::audio::devices
