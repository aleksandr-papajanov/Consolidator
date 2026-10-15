#pragma once

#include <array>

#include "../../Primitives/ParameterRamp.h"
#include "../../Primitives/Filters/FirstOrderLowPass.h"
#include "../../Core/AudioBuffer.h"

namespace consolidator::audio::algorithms
{

class SaturationProcessor final
{
public:
    void Prepare(const ProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetControls(double drive, double curve, bool split) noexcept;
    void Process(AudioBuffer audio, DetectorBuffer detector, ChannelMode mode) noexcept;

private:
    struct Channel
    {
        primitives::filters::FirstOrderLowPass low;
        primitives::filters::FirstOrderLowPass mid;
        std::array<double, 4> previousSamples{};
    };

    primitives::ParameterRamp drive_;
    primitives::ParameterRamp curve_{ 0.5 };
    primitives::ParameterRamp splitMix_;
    std::array<Channel, 2> channels_{};
    double detectorEnvelope_{};
    double envelopeReleaseCoefficient_{};
    double envelopeWeight_{};
};

} // namespace consolidator::audio::algorithms
