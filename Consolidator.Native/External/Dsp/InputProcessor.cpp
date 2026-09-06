#include "InputProcessor.h"

#include <cstddef>

namespace consolidator::max::dsp
{

void InputProcessor::Prepare(const DspProcessingContext& context)
{
    levelerDetector_.Prepare(context);
    leveler_.Prepare(context);
    width_.Prepare(context);
}

void InputProcessor::Reset() noexcept
{
    levelerDetector_.Reset();
    leveler_.Reset();
    width_.Reset();
}

void InputProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    levelerDetector_.SetParameters(snapshot);
    leveler_.SetParameters(snapshot);
    width_.SetTarget(snapshot.inputWidth);
    if (snapshot.inputLeveler == 0)
    {
        levelerDetector_.Reset();
    }
}

void InputProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    ProcessWidth(frame);
    if (frame.policy.levelerEnabled)
    {
        levelerDetector_.Process(frame);
    }
    leveler_.Process(frame);
}

bool InputProcessor::RequiresProcessing() const noexcept
{
    return leveler_.RequiresProcessing() ||
        width_.IsSmoothing() ||
        width_.CurrentValue() != 1.0;
}

bool InputProcessor::IsLevelerEnabled() const noexcept
{
    return leveler_.IsEnabled();
}

float InputProcessor::SlowInputLevelDb() const noexcept
{
    return leveler_.SlowInputLevelDb();
}

float InputProcessor::EffectiveInputLevelDb() const noexcept
{
    return leveler_.EffectiveInputLevelDb();
}

float InputProcessor::FastInputLevelDb() const noexcept
{
    return leveler_.FastInputLevelDb();
}

void InputProcessor::ProcessWidth(
    DspProcessingFrame& frame) noexcept
{
    if (!width_.IsSmoothing() && width_.CurrentValue() == 1.0)
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
            const auto side = (left - right) * 0.5 * width;
            frame.main.left[index] = middle + side;
            frame.main.right[index] = middle - side;
        }
    }
}

} // namespace consolidator::max::dsp
