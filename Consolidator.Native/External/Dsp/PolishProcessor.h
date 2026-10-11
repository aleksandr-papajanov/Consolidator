#pragma once

#include "DspProcessor.h"
#include "Processors/Air/PolishAirProcessor.h"
#include "Processors/Thick/PolishThickProcessor.h"

namespace consolidator::max::dsp
{

class PolishProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    processors::PolishThickProcessor thick_;
    processors::PolishAirProcessor air_;
};

} // namespace consolidator::max::dsp
