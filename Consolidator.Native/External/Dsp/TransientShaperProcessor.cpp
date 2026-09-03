#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void TransientShaperProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    attack_.Prepare(context);
    sustain_.Prepare(context);
}

void TransientShaperProcessor::Reset() noexcept
{
    attack_.Reset();
    sustain_.Reset();
}

void TransientShaperProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    attack_.SetTarget(snapshot.compressorAttack);
    sustain_.SetTarget(snapshot.compressorSustain);
}

void TransientShaperProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    attack_.Advance(frame.main.frameCount);
    sustain_.Advance(frame.main.frameCount);
}

} // namespace consolidator::max::dsp
