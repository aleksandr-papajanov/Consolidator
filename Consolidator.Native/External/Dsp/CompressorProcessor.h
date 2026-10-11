#pragma once

#include "CompressionProcessor.h"
#include "DetectorProcessor.h"
#include "DspProcessor.h"
#include "Processors/TransientShaper/TransientShaperProcessor.h"

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
    DetectorProcessor compressionDetector_{ 4 };
    processors::TransientShaperProcessor transientShaper_;
    CompressionProcessor compression_;
};

} // namespace consolidator::max::dsp
