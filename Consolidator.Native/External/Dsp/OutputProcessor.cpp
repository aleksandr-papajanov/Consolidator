#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void OutputProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    level_.Prepare(context);
    limiter_.Prepare(context);
}

void OutputProcessor::Reset() noexcept
{
    level_.Reset();
    limiter_.Reset();
}

void OutputProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    level_.SetTargetDb(snapshot.outputLevel);
    limiter_.SetParameters(snapshot);
    limiterEnabled_ = snapshot.outputLimiter != 0;
}

void OutputProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    level_.Process(frame.main, frame.plan.channelMode);
    if (limiterEnabled_)
    {
        limiter_.Process(frame);
    }
}

bool OutputProcessor::RequiresProcessing() const noexcept
{
    return level_.RequiresProcessing();
}

} // namespace consolidator::max::dsp
