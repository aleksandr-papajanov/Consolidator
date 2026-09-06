#pragma once

#include "DspGainRamp.h"
#include "DspProcessor.h"
#include "LimiterProcessor.h"

namespace consolidator::max::dsp
{

class OutputProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    DspGainRamp level_;
    LimiterProcessor limiter_;
};

} // namespace consolidator::max::dsp
