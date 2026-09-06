#pragma once

#include <array>

#include "DspAutoGain.h"
#include "DspParameterRamp.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

class SaturationProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    struct Channel
    {
        double low{};
        double mid{};
        std::array<double, 4> previousSamples{};
    };

    DspParameterRamp drive_;
    DspParameterRamp curve_{ 0.5 };
    DspParameterRamp splitMix_;
    DspAutoGain autoGain_;
    std::array<Channel, 2> channels_{};
    double detectorEnvelope_{};
    double envelopeReleaseCoefficient_{};
    double envelopeWeight_{};
    double lowCrossoverCoefficient_{};
    double highCrossoverCoefficient_{};
};

} // namespace consolidator::max::dsp
