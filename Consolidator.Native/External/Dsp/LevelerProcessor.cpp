#include "DspProcessors.h"

#include <algorithm>
#include <cmath>

namespace consolidator::max::dsp
{

namespace
{

constexpr double kSlowEnergyTimeConstantSeconds = 1.5;
constexpr double kFastEnergyTimeConstantSeconds = 0.25;
constexpr double kSlowTimeConstantSeconds = 6.0;
constexpr double kFastTimeConstantSeconds = 0.4;
constexpr double kMaximumFastCorrectionDb = 6.0;
constexpr double kSlowGateCloseEnergy = 1.0e-6;
constexpr double kSlowGateOpenEnergy = 1.9952623149688787e-6;
constexpr double kDecibelsPerAmplitudeDoubling = 6.020599913279624;
constexpr double kProcessingEpsilonDb = 1.0e-6;

}

void LevelerProcessor::Prepare(
    const DspProcessingContext& context) noexcept
{
    targetDb_.Prepare(context);
    inputGainDb_.Prepare(context);
    gateCoefficient_ = 1.0 - std::exp(-1.0 / (context.sampleRate * 0.050));
    gateHoldSamples_ = static_cast<std::size_t>(context.sampleRate * 0.200);
    slowEnergyCoefficient_ = std::exp(
        -1.0 / (context.sampleRate * kSlowEnergyTimeConstantSeconds));
    fastEnergyCoefficient_ = std::exp(
        -1.0 / (context.sampleRate * kFastEnergyTimeConstantSeconds));
    slowTimeConstantSamples_ = std::max(
        1.0,
        context.sampleRate * kSlowTimeConstantSeconds);
    fastTimeConstantSamples_ = std::max(
        1.0,
        context.sampleRate * kFastTimeConstantSeconds);
}

void LevelerProcessor::Reset() noexcept
{
    targetDb_.Reset();
    inputGainDb_.Reset();
    smoothedSlowEnergy_ = 0.0;
    smoothedFastEnergy_ = 0.0;
    effectiveGainDb_ = inputGainDb_.CurrentValue();
    slowGainDb_ = effectiveGainDb_;
    fastCorrectionDb_ = 0.0;
    slowGateOpen_ = false;
    hasSlowEnergyEstimate_ = false;
    hasFastEnergyEstimate_ = false;
    gateEnergy_ = 0.0;
    gateHoldRemaining_ = 0;
}

void LevelerProcessor::SetParameters(
    const DspSnapshot& snapshot) noexcept
{
    targetDb_.SetTarget(snapshot.inputTarget);
    const auto enabled = snapshot.inputLeveler != 0;
    if (enabled && !enabled_)
    {
        smoothedSlowEnergy_ = 0.0;
        smoothedFastEnergy_ = 0.0;
        slowGateOpen_ = false;
        hasSlowEnergyEstimate_ = false;
        hasFastEnergyEstimate_ = false;
        gateEnergy_ = 0.0;
        gateHoldRemaining_ = 0;
        slowGainDb_ = effectiveGainDb_;
        fastCorrectionDb_ = 0.0;
    }

    if (parametersInitialized_ && enabled_ && !enabled)
    {
        inputGainDb_.SetTarget(effectiveGainDb_);
        inputGainDb_.Reset();
        slowGainDb_ = effectiveGainDb_;
        fastCorrectionDb_ = 0.0;
    }

    inputGainDb_.SetTarget(snapshot.inputLevel);
    enabled_ = enabled;
    parametersInitialized_ = true;
}

void LevelerProcessor::Process(
    DspProcessingFrame& frame) noexcept
{
    const auto frameCount = frame.main.frameCount;
    targetDb_.Advance(frameCount);

    auto nextGainDb = effectiveGainDb_;
    auto nextSlowGainDb = slowGainDb_;
    auto nextFastCorrectionDb = fastCorrectionDb_;
    if (enabled_ && frameCount > 0)
    {
        const auto processLeft = frame.plan.channelMode != DspChannelMode::Right &&
            frame.detectorSignal.left != nullptr;
        const auto processRight = frame.plan.channelMode != DspChannelMode::Left &&
            frame.detectorSignal.right != nullptr;
        const auto channelCount = static_cast<double>(processLeft) +
            static_cast<double>(processRight);

        if (channelCount > 0.0)
        {
            auto blockEnergy = 0.0;
            for (std::size_t index = 0; index < frameCount; ++index)
            {
                auto sampleEnergy = 0.0;
                if (processLeft)
                {
                    const auto sample = frame.detectorSignal.left[index];
                    sampleEnergy += sample * sample;
                }
                if (processRight)
                {
                    const auto sample = frame.detectorSignal.right[index];
                    sampleEnergy += sample * sample;
                }
                sampleEnergy /= channelCount;
                blockEnergy += sampleEnergy;
                gateEnergy_ += gateCoefficient_ * (sampleEnergy - gateEnergy_);
                if (gateEnergy_ >= kSlowGateOpenEnergy)
                {
                    slowGateOpen_ = true;
                    gateHoldRemaining_ = gateHoldSamples_;
                }
                else if (gateEnergy_ <= kSlowGateCloseEnergy)
                {
                    if (gateHoldRemaining_ > 0)
                    {
                        --gateHoldRemaining_;
                    }
                    else
                    {
                        slowGateOpen_ = false;
                    }
                }
                else if (slowGateOpen_)
                {
                    gateHoldRemaining_ = gateHoldSamples_;
                }
                if (slowGateOpen_)
                {
                    smoothedFastEnergy_ = hasFastEnergyEstimate_
                        ? fastEnergyCoefficient_ * smoothedFastEnergy_ +
                            (1.0 - fastEnergyCoefficient_) * sampleEnergy
                        : std::max(gateEnergy_, kSilenceEnergy);
                    smoothedSlowEnergy_ = hasSlowEnergyEstimate_
                        ? slowEnergyCoefficient_ * smoothedSlowEnergy_ +
                            (1.0 - slowEnergyCoefficient_) * sampleEnergy
                        : std::max(gateEnergy_, kSilenceEnergy);
                    hasFastEnergyEstimate_ = true;
                    hasSlowEnergyEstimate_ = true;
                }
            }

            blockEnergy /= static_cast<double>(frameCount);

            auto desiredFastCorrectionDb = 0.0;
            if (slowGateOpen_ && blockEnergy >= kSilenceEnergy)
            {
                const auto fastMeasuredDb = 10.0 *
                    std::log10(std::max(smoothedFastEnergy_, kSilenceEnergy));

                const auto slowMeasuredDb = 10.0 *
                    std::log10(std::max(smoothedSlowEnergy_, kSilenceEnergy));
                const auto desiredSlowGainDb = std::clamp(
                    targetDb_.CurrentValue() - slowMeasuredDb,
                    kMinimumGainDb,
                    kMaximumGainDb);
                const auto slowCoefficient = std::exp(
                    -static_cast<double>(frameCount) /
                    slowTimeConstantSamples_);
                nextSlowGainDb = slowCoefficient * slowGainDb_ +
                    (1.0 - slowCoefficient) * desiredSlowGainDb;

                desiredFastCorrectionDb = std::clamp(
                    slowMeasuredDb - fastMeasuredDb,
                    -kMaximumFastCorrectionDb,
                    kMaximumFastCorrectionDb);
            }

            const auto minimumFastCorrectionDb = std::max(
                -kMaximumFastCorrectionDb,
                kMinimumGainDb - nextSlowGainDb);
            const auto maximumFastCorrectionDb = std::min(
                kMaximumFastCorrectionDb,
                kMaximumGainDb - nextSlowGainDb);
            const auto fastCoefficient = std::exp(
                -static_cast<double>(frameCount) /
                (fastTimeConstantSamples_ *
                    (desiredFastCorrectionDb > fastCorrectionDb_ ? 2.0 : 1.0)));
            nextFastCorrectionDb = std::clamp(
                fastCoefficient * fastCorrectionDb_ +
                    (1.0 - fastCoefficient) * desiredFastCorrectionDb,
                minimumFastCorrectionDb,
                maximumFastCorrectionDb);
            nextGainDb = nextSlowGainDb + nextFastCorrectionDb;
        }
    }
    if (!enabled_)
    {
        const auto processLeft = frame.plan.channelMode != DspChannelMode::Right;
        const auto processRight = frame.plan.channelMode != DspChannelMode::Left;
        if (!inputGainDb_.IsSmoothing())
        {
            const auto gain = std::exp2(
                inputGainDb_.CurrentValue() /
                kDecibelsPerAmplitudeDoubling);
            for (std::size_t index = 0; index < frameCount; ++index)
            {
                if (processLeft && frame.main.left != nullptr)
                {
                    frame.main.left[index] *= gain;
                }
                if (processRight && frame.main.right != nullptr)
                {
                    frame.main.right[index] *= gain;
                }
            }

            effectiveGainDb_ = inputGainDb_.CurrentValue();
            slowGainDb_ = effectiveGainDb_;
            fastCorrectionDb_ = 0.0;
            return;
        }

        for (std::size_t index = 0; index < frameCount; ++index)
        {
            const auto gain = std::exp2(
                inputGainDb_.NextValue() /
                kDecibelsPerAmplitudeDoubling);
            if (processLeft && frame.main.left != nullptr)
            {
                frame.main.left[index] *= gain;
            }
            if (processRight && frame.main.right != nullptr)
            {
                frame.main.right[index] *= gain;
            }
        }

        effectiveGainDb_ = inputGainDb_.CurrentValue();
        slowGainDb_ = effectiveGainDb_;
        fastCorrectionDb_ = 0.0;
        return;
    }

    const auto processLeft = frame.plan.channelMode != DspChannelMode::Right;
    const auto processRight = frame.plan.channelMode != DspChannelMode::Left;
    const auto startGain = std::exp2(
        effectiveGainDb_ / kDecibelsPerAmplitudeDoubling);
    const auto endGain = std::exp2(
        nextGainDb / kDecibelsPerAmplitudeDoubling);
    const auto gainMultiplier = frameCount > 0 && startGain > 0.0
        ? std::pow(endGain / startGain, 1.0 / static_cast<double>(frameCount))
        : 1.0;
    auto gain = startGain;

    for (std::size_t index = 0; index < frameCount; ++index)
    {
        if (processLeft && frame.main.left != nullptr)
        {
            frame.main.left[index] *= gain;
        }
        if (processRight && frame.main.right != nullptr)
        {
            frame.main.right[index] *= gain;
        }
        gain *= gainMultiplier;
    }

    effectiveGainDb_ = nextGainDb;
    slowGainDb_ = nextSlowGainDb;
    fastCorrectionDb_ = nextFastCorrectionDb;
}

bool LevelerProcessor::RequiresProcessing() const noexcept
{
    return enabled_ || inputGainDb_.IsSmoothing() ||
        std::abs(effectiveGainDb_) > kProcessingEpsilonDb;
}

bool LevelerProcessor::IsEnabled() const noexcept
{
    return enabled_;
}

float LevelerProcessor::SlowInputLevelDb() const noexcept
{
    return static_cast<float>(slowGainDb_);
}

float LevelerProcessor::EffectiveInputLevelDb() const noexcept
{
    return static_cast<float>(effectiveGainDb_);
}

float LevelerProcessor::FastInputLevelDb() const noexcept
{
    return static_cast<float>(fastCorrectionDb_);
}

} // namespace consolidator::max::dsp
