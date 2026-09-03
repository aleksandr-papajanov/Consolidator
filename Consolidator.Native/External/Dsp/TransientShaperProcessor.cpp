#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void TransientShaperProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    (void)context;
}

void TransientShaperProcessor::Reset() noexcept
{
}

void TransientShaperProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    attack_ = snapshot.compressorAttack;
    sustain_ = snapshot.compressorSustain;
}

void TransientShaperProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    (void)frame;
}

} // namespace consolidator::max::dsp
