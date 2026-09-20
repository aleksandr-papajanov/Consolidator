#pragma once

#include "DspProcessor.h"
#include "PolishAirProcessor.h"
#include "PolishThickProcessor.h"

namespace consolidator::max::dsp
{

class PolishProcessor final : public DspProcessor
{
public:
    void SetTuningsBeforePrepare(
        const PolishThickTuning& thick,
        const PolishAirTuning& air) noexcept;
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    PolishThickProcessor thick_;
    PolishAirProcessor air_;
};

} // namespace consolidator::max::dsp
