#include "DspProcessors.h"

#include <algorithm>
#include <cmath>

namespace consolidator::max::dsp
{

void InputProcessor::Prepare(const DspProcessingContext& context)
{
    level_.Prepare(context);
    levelerDetector_.Prepare(context);
    leveler_.Prepare(context);
    const auto rampSamples = std::round(context.sampleRate * 0.01);
    widthRampSampleCount_ = static_cast<std::size_t>(
        std::max(1.0, rampSamples));
}

void InputProcessor::Reset() noexcept
{
    level_.Reset();
    levelerDetector_.Reset();
    leveler_.Reset();
    currentWidth_ = targetWidth_;
    widthStep_ = 0.0F;
    widthRemainingSamples_ = 0;
}

void InputProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    level_.SetTargetDb(snapshot.inputLevel);
    levelerDetector_.SetParameters(snapshot);
    leveler_.SetParameters(snapshot);
    if (snapshot.inputWidth == targetWidth_)
    {
        return;
    }

    targetWidth_ = snapshot.inputWidth;
    if (targetWidth_ == currentWidth_)
    {
        widthStep_ = 0.0F;
        widthRemainingSamples_ = 0;
        return;
    }

    widthRemainingSamples_ = widthRampSampleCount_;
    widthStep_ = (targetWidth_ - currentWidth_) /
        static_cast<float>(widthRemainingSamples_);
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
        widthRemainingSamples_ > 0 ||
        currentWidth_ != 100.0F;
}

void InputProcessor::ProcessWidth(
    DspProcessingFrame& frame) noexcept
{
    if (widthRemainingSamples_ == 0 && currentWidth_ == 100.0F)
    {
        return;
    }

    const auto stereo = frame.plan.channelMode == DspChannelMode::Stereo &&
        frame.main.left != nullptr &&
        frame.main.right != nullptr;
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        if (stereo)
        {
            const auto left = frame.main.left[index];
            const auto right = frame.main.right[index];
            const auto middle = (left + right) * 0.5;
            const auto side = (left - right) * 0.5 *
                (static_cast<double>(currentWidth_) / 100.0);
            frame.main.left[index] = middle + side;
            frame.main.right[index] = middle - side;
        }

        if (widthRemainingSamples_ > 0)
        {
            currentWidth_ += widthStep_;
            --widthRemainingSamples_;
            if (widthRemainingSamples_ == 0)
            {
                currentWidth_ = targetWidth_;
                widthStep_ = 0.0F;
            }
        }
    }
}

} // namespace consolidator::max::dsp
