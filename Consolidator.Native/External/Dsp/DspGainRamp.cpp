#include "DspProcessors.h"

#include <algorithm>
#include <cmath>

namespace consolidator::max::dsp
{

void DspAutoGain::Prepare(const DspProcessingContext& context) noexcept
{
    energyCoefficient_ = 1.0 - std::exp(-1.0 / context.sampleRate);
    gainCoefficient_ = 1.0 - std::exp(-1.0 / (2.0 * context.sampleRate));
}

void DspAutoGain::Reset() noexcept
{
    inputEnergy_ = 0.0;
    outputEnergy_ = 0.0;
    correctionDb_ = 0.0;
}

double DspAutoGain::Process(double inputEnergy, double outputEnergy, double activity) noexcept
{
    if (activity <= 0.0)
    {
        Reset();
        return 1.0;
    }
    if (inputEnergy > 1.0e-10)
    {
        inputEnergy_ += energyCoefficient_ * (inputEnergy - inputEnergy_);
        outputEnergy_ += energyCoefficient_ * (outputEnergy - outputEnergy_);
        if (inputEnergy_ > 1.0e-15 && outputEnergy_ > 1.0e-15)
        {
            const auto desiredDb = std::clamp(
                0.5 * 10.0 * std::log10(inputEnergy_ / outputEnergy_), -3.0, 3.0);
            correctionDb_ += gainCoefficient_ * (desiredDb - correctionDb_);
        }
    }
    return std::pow(10.0, std::clamp(activity, 0.0, 1.0) * correctionDb_ / 20.0);
}

void DspGainRamp::Prepare(
    const DspProcessingContext& context) noexcept
{
    gain_.Prepare(context);
}

void DspGainRamp::Reset() noexcept
{
    gain_.Reset();
}

void DspGainRamp::SetTargetDb(float targetDb) noexcept
{
    gain_.SetTarget(std::pow(
        10.0,
        static_cast<double>(targetDb) / 20.0));
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
        const auto gain = gain_.NextValue();
        if (processLeft && audio.left != nullptr)
        {
            audio.left[index] *= gain;
        }
        if (processRight && audio.right != nullptr)
        {
            audio.right[index] *= gain;
        }
    }
}

bool DspGainRamp::RequiresProcessing() const noexcept
{
    return gain_.IsSmoothing() || gain_.CurrentValue() != 1.0;
}

} // namespace consolidator::max::dsp
