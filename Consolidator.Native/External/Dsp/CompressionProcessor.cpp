#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void CompressionProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    amount_.Prepare(context);
}

void CompressionProcessor::Reset() noexcept
{
    amount_.Reset();
}

void CompressionProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    amount_.SetTarget(snapshot.compressorCompression);
    character_ = snapshot.compressorCharacter;
}

void CompressionProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    amount_.Advance(frame.main.frameCount);
}

} // namespace consolidator::max::dsp
