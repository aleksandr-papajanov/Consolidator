#include "CompressorProcessor.h"

namespace consolidator::max::dsp
{

void CompressorProcessor::Prepare(const DspProcessingContext& context)
{
    compressionDetector_.Prepare(context);
    transientShaper_.Prepare(context);
    compression_.Prepare(context);
}

void CompressorProcessor::Reset() noexcept
{
    compressionDetector_.Reset();
    transientShaper_.Reset();
    compression_.Reset();
}

void CompressorProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    compressionDetector_.SetParameters(snapshot);
    transientShaper_.SetParameters(snapshot);
    compression_.SetParameters(snapshot);
}

void CompressorProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    compressionDetector_.Process(frame);
    compression_.Process(frame);
    transientShaper_.Process(frame);
}

} // namespace consolidator::max::dsp
