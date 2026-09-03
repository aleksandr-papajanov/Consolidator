#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void CompressionProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    (void)context;
}

void CompressionProcessor::Reset() noexcept
{
}

void CompressionProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    amount_ = snapshot.compressorCompression;
    character_ = snapshot.compressorCharacter;
}

void CompressionProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    (void)frame;
}

} // namespace consolidator::max::dsp
