#include "DspProcessors.h"

namespace consolidator::max::dsp
{

void InputProcessor::Prepare(const DspProcessingContext& context)
{
    level_.Prepare(context);
    levelerDetector_.Prepare(context);
    leveler_.Prepare(context);
    width_.Prepare(context);
}

void InputProcessor::Reset() noexcept
{
    level_.Reset();
    levelerDetector_.Reset();
    leveler_.Reset();
    width_.Reset();
}

void InputProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    level_.SetTargetDb(snapshot.inputLevel);
    levelerDetector_.SetParameters(snapshot);
    leveler_.SetParameters(snapshot);
    if (snapshot.inputLeveler == 0)
    {
        leveler_.Reset();
    }
    width_.SetTarget(snapshot.inputWidth);
}

void InputProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    ProcessWidth(frame);
    level_.Process(frame.main, frame.plan.channelMode);
    if (frame.policy.levelerEnabled)
    {
        levelerDetector_.Process(frame);
        leveler_.Process(frame);
    }
}

bool InputProcessor::RequiresProcessing() const noexcept
{
    return level_.RequiresProcessing() ||
        width_.IsSmoothing() ||
        width_.CurrentValue() != 100.0;
}

void InputProcessor::ProcessWidth(
    DspProcessingFrame& frame) noexcept
{
    if (!width_.IsSmoothing() && width_.CurrentValue() == 100.0)
    {
        return;
    }

    const auto stereo = frame.plan.channelMode == DspChannelMode::Stereo &&
        frame.main.left != nullptr &&
        frame.main.right != nullptr;
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        const auto width = width_.NextValue();
        if (stereo)
        {
            const auto left = frame.main.left[index];
            const auto right = frame.main.right[index];
            const auto middle = (left + right) * 0.5;
            const auto side = (left - right) * 0.5 *
                (width / 100.0);
            frame.main.left[index] = middle + side;
            frame.main.right[index] = middle - side;
        }
    }
}

} // namespace consolidator::max::dsp
