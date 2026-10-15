#pragma once

#include "DetectorEqualizer.h"
#include "WetTransition.h"
#include "../Algorithms/Saturation/SaturationProcessor.h"
#include "../Primitives/GainRamp.h"

namespace consolidator::audio::devices
{

struct SaturatorParameters
{
    double drive{};
    double curve{};
    double outputDb{};
    bool split{};
    bool bypass{};
    std::array<FilterParameters, 2> detector{};
};

class SaturatorDevice final
{
public:
    void Prepare(const ProcessingContext& context);
    void Reset() noexcept;
    void SetParameters(const SaturatorParameters& parameters) noexcept;
    void Process(AudioBuffer audio, ChannelMode mode) noexcept;

private:
    DetectorEqualizer detector_;
    algorithms::SaturationProcessor saturation_;
    primitives::GainRamp output_;
    WetTransition transition_;
};

} // namespace consolidator::audio::devices
