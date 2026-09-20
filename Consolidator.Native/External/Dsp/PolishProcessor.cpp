#include "PolishProcessor.h"

namespace consolidator::max::dsp
{

void PolishProcessor::SetTuningsBeforePrepare(
    const PolishThickTuning& thick,
    const PolishAirTuning& air) noexcept
{
    thick_.SetTuningBeforePrepare(thick);
    air_.SetTuningBeforePrepare(air);
}

void PolishProcessor::Prepare(const DspProcessingContext& context)
{
    thick_.Prepare(context);
    air_.Prepare(context);
}

void PolishProcessor::Reset() noexcept
{
    thick_.Reset();
    air_.Reset();
}

void PolishProcessor::SetParameters(const DspSnapshot& snapshot) noexcept
{
    thick_.SetParameters(snapshot);
    air_.SetParameters(snapshot);
}

void PolishProcessor::Process(DspProcessingFrame& frame) noexcept
{
    // Each processor adds its own enhancement to the incoming audio.
    // Serial composition keeps both controls at their intended intensity.
    thick_.Process(frame);
    air_.Process(frame);
}

} // namespace consolidator::max::dsp
