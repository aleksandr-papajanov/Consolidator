#pragma once

#include "CompressionProcessor.h"
#include "DetectorProcessor.h"
#include "DspGainRamp.h"
#include "DspProcessor.h"
#include "TransientShaperProcessor.h"

namespace consolidator::max::dsp
{

class CompressorProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    DetectorProcessor detector_{ 4 };
    DetectorProcessor compressionDetector_{ 4 };
    TransientShaperProcessor transientShaper_;
    CompressionProcessor compression_;
    DspGainRamp output_;
};

} // namespace consolidator::max::dsp
