#include "DspProcessors.h"

#include <algorithm>
#include <cmath>

namespace consolidator::max::dsp
{

void DspGainRamp::Prepare(
    const DspProcessingContext& context) noexcept
{
    const auto rampSamples = std::round(context.sampleRate * 0.01);
    rampSampleCount_ = static_cast<std::size_t>(
        std::max(1.0, rampSamples));
}

void DspGainRamp::Reset() noexcept
{
    currentGain_ = targetGain_;
    gainRatio_ = 1.0;
    remainingSamples_ = 0;
}

void DspGainRamp::SetTargetDb(float targetDb) noexcept
{
    const auto targetGain = std::pow(
        10.0,
        static_cast<double>(targetDb) / 20.0);
    if (targetGain == targetGain_)
    {
        return;
    }

    targetGain_ = targetGain;
    if (targetGain_ == currentGain_)
    {
        gainRatio_ = 1.0;
        remainingSamples_ = 0;
        return;
    }

    gainRatio_ = std::pow(
        targetGain_ / currentGain_,
        1.0 / static_cast<double>(rampSampleCount_));
    remainingSamples_ = rampSampleCount_;
}

void DspGainRamp::Process(
    DspAudioBuffer& audio,
    DspChannelMode channelMode) noexcept
{
    if (!RequiresProcessing())
    {
        return;
    }

    const auto processLeft = channelMode != DspChannelMode::Right;
    const auto processRight = channelMode != DspChannelMode::Left;
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        if (processLeft && audio.left != nullptr)
        {
            audio.left[index] *= currentGain_;
        }
        if (processRight && audio.right != nullptr)
        {
            audio.right[index] *= currentGain_;
        }

        if (remainingSamples_ > 0)
        {
            currentGain_ *= gainRatio_;
            --remainingSamples_;
            if (remainingSamples_ == 0)
            {
                currentGain_ = targetGain_;
                gainRatio_ = 1.0;
            }
        }
    }
}

bool DspGainRamp::RequiresProcessing() const noexcept
{
    return remainingSamples_ > 0 || currentGain_ != 1.0;
}

} // namespace consolidator::max::dsp
