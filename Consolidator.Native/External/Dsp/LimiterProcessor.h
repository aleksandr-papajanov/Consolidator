#pragma once

#include "DspParameterRamp.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

class LimiterProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;
    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    DspParameterRamp targetDb_{ -1.0 };
    double gain_{ 1.0 };
    double releaseCoefficient_{};
    bool enabled_{};
};

} // namespace consolidator::max::dsp
