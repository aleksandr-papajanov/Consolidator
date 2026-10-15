#include "LevelerProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>

namespace consolidator::audio::algorithms
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

}

void LevelerProcessor::Prepare(
    const ProcessingContext& context) noexcept
{
    weighting_.Prepare(context.sampleRate);
    targetDb_.Prepare(context);
    manualGainDb_.Prepare(context);
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
    weighting_.Reset();
    targetDb_.Reset();
    manualGainDb_.Reset();
    smoothedSlowEnergy_ = 0.0;
    smoothedFastEnergy_ = 0.0;
    effectiveGainDb_ = manualGainDb_.CurrentValue();
    slowGainDb_ = effectiveGainDb_;
    fastCorrectionDb_ = 0.0;
    slowGateOpen_ = false;
    hasSlowEnergyEstimate_ = false;
    hasFastEnergyEstimate_ = false;
    gateEnergy_ = 0.0;
    gateHoldRemaining_ = 0;
}

void LevelerProcessor::SetControls(
    double levelDb, double targetDb, bool enabled) noexcept
{
    targetDb_.SetTarget(targetDb);
    if (enabled && !enabled_)
    {
        weighting_.Reset();
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
        manualGainDb_.SetTarget(effectiveGainDb_);
        manualGainDb_.Reset();
        slowGainDb_ = effectiveGainDb_;
        fastCorrectionDb_ = 0.0;
    }

    manualGainDb_.SetTarget(levelDb);
    enabled_ = enabled;
    parametersInitialized_ = true;
}

void LevelerProcessor::Process(AudioBuffer audio, DetectorBuffer detector, ChannelMode mode) noexcept
{
    targetDb_.Advance(audio.frameCount);
    if (!enabled_)
    {
        ProcessManualGain(audio, mode);
        return;
    }
    const auto targets = CalculateAutomaticGain(detector, mode, audio.frameCount);
    ProcessAutomaticGain(audio, mode, targets);
}

LevelerProcessor::GainTargets LevelerProcessor::CalculateAutomaticGain(
    DetectorBuffer detector, ChannelMode mode, std::size_t frameCount) noexcept
{
    auto nextGainDb = effectiveGainDb_;
    auto nextSlowGainDb = slowGainDb_;
    auto nextFastCorrectionDb = fastCorrectionDb_;
    if (frameCount > 0)
    {
        const auto processLeft = mode != ChannelMode::Right &&
            detector.left != nullptr;
        const auto processRight = mode != ChannelMode::Left &&
            detector.right != nullptr;
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
                    const auto sample = weighting_.Process(detector.left[index], false);
                    sampleEnergy += sample * sample;
                }
                if (processRight)
                {
                    const auto sample = weighting_.Process(detector.right[index], true);
                    sampleEnergy += sample * sample;
                }
                sampleEnergy /= channelCount;
                blockEnergy += sampleEnergy;
                gateEnergy_ += gateCoefficient_ * (sampleEnergy - gateEnergy_);
                const auto gateWasOpen = slowGateOpen_;
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
                    const auto gateJustOpened = !gateWasOpen;
                    smoothedFastEnergy_ = hasFastEnergyEstimate_ && !gateJustOpened
                        ? fastEnergyCoefficient_ * smoothedFastEnergy_ +
                            (1.0 - fastEnergyCoefficient_) * sampleEnergy
                        : std::max(sampleEnergy, kSilenceEnergy);
                    smoothedSlowEnergy_ = hasSlowEnergyEstimate_ && !gateJustOpened
                        ? slowEnergyCoefficient_ * smoothedSlowEnergy_ +
                            (1.0 - slowEnergyCoefficient_) * sampleEnergy
                        : std::max(sampleEnergy, kSilenceEnergy);
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
    return { nextGainDb, nextSlowGainDb, nextFastCorrectionDb };
}

void LevelerProcessor::ProcessManualGain(AudioBuffer audio, ChannelMode mode) noexcept
{
    const auto frameCount = audio.frameCount;
    const auto processLeft = mode != ChannelMode::Right;
    const auto processRight = mode != ChannelMode::Left;
    if (!manualGainDb_.IsSmoothing())
    {
        const auto gain = std::exp2(
            manualGainDb_.CurrentValue() /
            kDecibelsPerAmplitudeDoubling);
        for (std::size_t index = 0; index < frameCount; ++index)
        {
            if (processLeft && audio.left != nullptr)
            {
                audio.left[index] *= gain;
            }
            if (processRight && audio.right != nullptr)
            {
                audio.right[index] *= gain;
            }
        }

        effectiveGainDb_ = manualGainDb_.CurrentValue();
        slowGainDb_ = effectiveGainDb_;
        fastCorrectionDb_ = 0.0;
        return;
    }

    for (std::size_t index = 0; index < frameCount; ++index)
    {
        const auto gain = std::exp2(
            manualGainDb_.NextValue() /
            kDecibelsPerAmplitudeDoubling);
        if (processLeft && audio.left != nullptr)
        {
            audio.left[index] *= gain;
        }
        if (processRight && audio.right != nullptr)
        {
            audio.right[index] *= gain;
        }
    }

    effectiveGainDb_ = manualGainDb_.CurrentValue();
    slowGainDb_ = effectiveGainDb_;
    fastCorrectionDb_ = 0.0;
}

void LevelerProcessor::ProcessAutomaticGain(
    AudioBuffer audio, ChannelMode mode, const GainTargets& targets) noexcept
{
    const auto frameCount = audio.frameCount;
    const auto processLeft = mode != ChannelMode::Right;
    const auto processRight = mode != ChannelMode::Left;
    const auto startGain = std::exp2(
        effectiveGainDb_ / kDecibelsPerAmplitudeDoubling);
    const auto endGain = std::exp2(
        targets.effectiveDb / kDecibelsPerAmplitudeDoubling);
    const auto gainMultiplier = frameCount > 0 && startGain > 0.0
        ? std::pow(endGain / startGain, 1.0 / static_cast<double>(frameCount))
        : 1.0;
    auto gain = startGain;

    for (std::size_t index = 0; index < frameCount; ++index)
    {
        if (processLeft && audio.left != nullptr)
        {
            audio.left[index] *= gain;
        }
        if (processRight && audio.right != nullptr)
        {
            audio.right[index] *= gain;
        }
        gain *= gainMultiplier;
    }

    effectiveGainDb_ = targets.effectiveDb;
    slowGainDb_ = targets.slowDb;
    fastCorrectionDb_ = targets.fastDb;
}

double LevelerProcessor::SlowGainDb() const noexcept
{
    return slowGainDb_;
}

double LevelerProcessor::FastCorrectionDb() const noexcept
{
    return fastCorrectionDb_;
}

} // namespace consolidator::audio::algorithms
