#include "DspProcessors.h"

namespace consolidator::max::dsp
{

namespace
{

void IgnoreProcess(
    DspProcessingFrame& frame) noexcept
{
    (void)frame;
}

void IgnorePrepare(const DspProcessingContext& context) noexcept
{
    (void)context;
}

void IgnoreSnapshot(const DspSnapshot& snapshot) noexcept
{
    (void)snapshot;
}

}

void InputProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    IgnorePrepare(context);
}

void InputProcessor::Reset() noexcept
{
    gain_ = 1.0;
}

void InputProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    const bool processLeft = frame.plan.channelMode != DspChannelMode::Right;
    const bool processRight = frame.plan.channelMode != DspChannelMode::Left;

    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        if (processLeft && frame.main.left != nullptr)
        {
            frame.main.left[index] *= gain_;
        }
        if (processRight && frame.main.right != nullptr)
        {
            frame.main.right[index] *= gain_;
        }
    }
}

void LevelerProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    IgnorePrepare(context);
}

void LevelerProcessor::Reset() noexcept
{
}

void LevelerProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    IgnoreSnapshot(snapshot);
}

void LevelerProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    IgnoreProcess(frame);
}

void EqualizerDetectorProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    IgnorePrepare(context);
}

void EqualizerDetectorProcessor::Reset() noexcept
{
}

void EqualizerDetectorProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    IgnoreSnapshot(snapshot);
}

void EqualizerDetectorProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    IgnoreProcess(frame);
}

void SaturatorProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    IgnorePrepare(context);
}

void SaturatorProcessor::Reset() noexcept
{
}

void SaturatorProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    IgnoreSnapshot(snapshot);
}

void SaturatorProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    IgnoreProcess(frame);
}

void CompressorProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    IgnorePrepare(context);
}

void CompressorProcessor::Reset() noexcept
{
}

void CompressorProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    IgnoreSnapshot(snapshot);
}

void CompressorProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    IgnoreProcess(frame);
}

void EqualizerProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    IgnorePrepare(context);
}

void EqualizerProcessor::Reset() noexcept
{
}

void EqualizerProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    IgnoreSnapshot(snapshot);
}

void EqualizerProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    IgnoreProcess(frame);
}

void PolishProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    IgnorePrepare(context);
}

void PolishProcessor::Reset() noexcept
{
}

void PolishProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    IgnoreSnapshot(snapshot);
}

void PolishProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    IgnoreProcess(frame);
}

void OutputProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    IgnorePrepare(context);
}

void OutputProcessor::Reset() noexcept
{
}

void OutputProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    IgnoreSnapshot(snapshot);
}

void OutputProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    IgnoreProcess(frame);
}

void LimiterProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    IgnorePrepare(context);
}

void LimiterProcessor::Reset() noexcept
{
}

void LimiterProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    IgnoreSnapshot(snapshot);
}

void LimiterProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    IgnoreProcess(frame);
}

} // namespace consolidator::max::dsp
