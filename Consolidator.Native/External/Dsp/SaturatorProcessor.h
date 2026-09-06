#pragma once

#include "DetectorProcessor.h"
#include "DspGainRamp.h"
#include "DspProcessor.h"
#include "SaturationProcessor.h"

namespace consolidator::max::dsp
{

class SaturatorProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    DetectorProcessor detector_{ 2 };
    SaturationProcessor saturation_;
    DspGainRamp output_;
};

} // namespace consolidator::max::dsp
