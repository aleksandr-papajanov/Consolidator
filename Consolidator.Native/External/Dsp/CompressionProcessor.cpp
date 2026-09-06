#include "CompressionProcessor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "DspMath.h"
#include "DspProcessingPrimitives.h"

namespace consolidator::max::dsp
{

void CompressionProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    amount_.Prepare(context);
    activeMix_.Prepare(context);
    attackSeconds_.Prepare(context);
    releaseSeconds_.Prepare(context);
    sampleRate_ = context.sampleRate;
    detectorCoefficient_ = TimeCoefficient(0.002, sampleRate_);
    referenceCoefficient_ = TimeCoefficient(0.500, sampleRate_);
    meterCoefficient_ = TimeCoefficient(1.0, sampleRate_);
    makeupCoefficient_ = TimeCoefficient(1.500, sampleRate_);
}

void CompressionProcessor::Reset() noexcept
{
    amount_.Reset();
    activeMix_.Reset();
    attackSeconds_.Reset();
    releaseSeconds_.Reset();
    detectorEnergy_ = 0.0;
    referenceEnergy_ = 0.0;
    inputEnergy_ = 0.0;
    outputEnergy_ = 0.0;
    reductionDb_ = 0.0;
    makeupDb_ = 0.0;
    reductionDuration_ = 0.0;
    measuredReductionDb_ = 0.0;
    reductionCorrectionDb_ = 0.0;
    referenceInitialized_ = false;
}

void CompressionProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    amount_.SetTarget(snapshot.compressorCompression);
    activeMix_.SetTarget(snapshot.compressorCompression > 0.0F ? 1.0 : 0.0);
    // Existing UI order: Punch, Tight, Smooth.
    switch (snapshot.compressorCharacter)
    {
        case 1:
        {
            attackSeconds_.SetTarget(0.001);
            releaseSeconds_.SetTarget(0.045);
            break;
        }
        case 2:
        {
            attackSeconds_.SetTarget(0.010);
            releaseSeconds_.SetTarget(0.250);
            break;
        }
        default:
        {
            attackSeconds_.SetTarget(0.025);
            releaseSeconds_.SetTarget(0.060);
            break;
        }
    }
}

void CompressionProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    const auto selected = SelectChannels(frame.main, frame.plan.channelMode);
    const auto left = selected.left != nullptr;
    const auto right = selected.right != nullptr;
    const auto channels = static_cast<double>(left) + static_cast<double>(right);
    if (channels == 0.0)
    {
        return;
    }
    for (std::size_t index = 0; index < frame.main.frameCount; ++index)
    {
        const auto amount = std::clamp(amount_.NextValue(), 0.0, 1.0);
        const auto attackSeconds = attackSeconds_.NextValue();
        const auto baseRelease = releaseSeconds_.NextValue();
        auto energy = 0.0;
        const auto mainEnergy = selected.MeanSquare(index);
        if (left)
        {
            const auto detector = frame.detectorSignal.left == nullptr ? 0.0 : frame.detectorSignal.left[index];
            energy += detector * detector;
        }
        if (right)
        {
            const auto detector = frame.detectorSignal.right == nullptr ? 0.0 : frame.detectorSignal.right[index];
            energy += detector * detector;
        }
        energy /= channels;
        detectorEnergy_ += detectorCoefficient_ * (energy - detectorEnergy_);
        const auto signalPresent = detectorEnergy_ > 1.0e-9;
        if (signalPresent)
        {
            if (!referenceInitialized_)
            {
                referenceEnergy_ = detectorEnergy_;
                referenceInitialized_ = true;
            }
            referenceEnergy_ += referenceCoefficient_ * (energy - referenceEnergy_);
        }

        constexpr std::array<double, 5> targets{ 0.0, 2.0, 5.0, 9.0, 18.0 };
        const auto position = amount * 4.0;
        const auto segment = std::min(static_cast<std::size_t>(position), std::size_t{ 3 });
        const auto targetReduction = targets[segment] +
            (position - static_cast<double>(segment)) * (targets[segment + 1] - targets[segment]);
        const auto slope = 1.0 - 1.0 / (1.0 + 7.0 * amount);
        auto desiredReduction = 0.0;
        if (signalPresent && amount > 0.0 && referenceEnergy_ > 1.0e-12)
        {
            // Equivalent to threshold = referenceDb - targetGR / slope,
            // without dividing by a vanishing slope near neutral.
            const auto relativeDb = 10.0 * std::log10(detectorEnergy_ / referenceEnergy_);
            const auto over = targetReduction + slope * relativeDb + amount * reductionCorrectionDb_;
            const auto halfKnee = 3.0 * slope;
            if (over >= halfKnee)
            {
                desiredReduction = over;
            }
            else if (over > -halfKnee)
            {
                desiredReduction = (over + halfKnee) * (over + halfKnee) / (4.0 * halfKnee);
            }
            desiredReduction = std::min(desiredReduction, 30.0);
        }
        const auto durationTarget = desiredReduction > 1.0 ? 1.0 : 0.0;
        reductionDuration_ += referenceCoefficient_ * (durationTarget - reductionDuration_);
        const auto release = baseRelease * (1.0 + reductionDuration_);
        const auto time = desiredReduction > reductionDb_ ? attackSeconds : release;
        const auto coefficient = TimeCoefficient(time, sampleRate_);
        reductionDb_ += coefficient * (desiredReduction - reductionDb_);
        if (signalPresent && amount > 0.0)
        {
            measuredReductionDb_ += referenceCoefficient_ * (reductionDb_ - measuredReductionDb_);
            // Slow, bounded threshold trim corrects the timing-dependent average GR.
            reductionCorrectionDb_ = std::clamp(reductionCorrectionDb_ +
                (targetReduction - measuredReductionDb_) / (sampleRate_ * 3.0), -6.0, 6.0);
        }
        else if (amount == 0.0)
        {
            reductionCorrectionDb_ = 0.0;
            measuredReductionDb_ = 0.0;
        }
        const auto reductionGain = DbToGain(-reductionDb_);

        // Actual main-path energy before and after GR, excluding makeup.
        if (mainEnergy > 1.0e-9 && signalPresent)
        {
            inputEnergy_ += meterCoefficient_ * (mainEnergy - inputEnergy_);
            outputEnergy_ += meterCoefficient_ * (mainEnergy * reductionGain * reductionGain - outputEnergy_);
        }
        auto desiredMakeup = makeupDb_;
        if (amount == 0.0)
        {
            desiredMakeup = 0.0;
        }
        else if (signalPresent && inputEnergy_ > 1.0e-12 && outputEnergy_ > 1.0e-12)
        {
            desiredMakeup = std::clamp(0.75 * 10.0 * std::log10(inputEnergy_ / outputEnergy_), 0.0, 12.0);
        }
        makeupDb_ += makeupCoefficient_ * (desiredMakeup - makeupDb_);
        const auto activity = activeMix_.NextValue();
        const auto gain = DbToGain(activity * (makeupDb_ - reductionDb_));
        selected.ApplyGain(index, gain);
    }
}

} // namespace consolidator::max::dsp
