#include "CompressorProcessor.h"

namespace consolidator::max::dsp
{

void CompressorProcessor::Prepare(const DspProcessingContext& context)
{
    detector_.Prepare(context);
    compressionDetector_.Prepare(context);
    transientShaper_.Prepare(context);
    compression_.Prepare(context);
    output_.Prepare(context);
}

void CompressorProcessor::Reset() noexcept
{
    detector_.Reset();
    compressionDetector_.Reset();
    transientShaper_.Reset();
    compression_.Reset();
    output_.Reset();
}

void CompressorProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    detector_.SetParameters(snapshot);
    compressionDetector_.SetParameters(snapshot);
    transientShaper_.SetParameters(snapshot);
    compression_.SetParameters(snapshot);
    output_.SetTargetDb(snapshot.compressorOutputDb);
}

void CompressorProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    compressionDetector_.Process(frame);
    compression_.Process(frame);
    detector_.Process(frame);
    transientShaper_.Process(frame);
    output_.Process(frame.main, frame.plan.channelMode);
}

} // namespace consolidator::max::dsp
