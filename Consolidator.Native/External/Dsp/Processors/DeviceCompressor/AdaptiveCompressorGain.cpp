#include "AdaptiveCompressorGain.h"

#include <algorithm>
#include <cmath>
#include <initializer_list>
#include <numbers>

#include "../../DspMath.h"
#include "../../DspProcessingPrimitives.h"
#include "../CompressorControlMode.h"

namespace consolidator::max::dsp::processors
{

namespace
{

constexpr double kMeasurementIntervalSeconds = 0.02;
constexpr double kPowerAverageSeconds = 3.0;
constexpr double kCorrectionSeconds = 4.0;
constexpr double kWarmupSeconds = 0.5;
constexpr double kDeadbandDb = 0.3;
constexpr double kMinimumReferencePower = 1.0e-6; // -60 dBFS weighted mean-square.
constexpr double kRelativeActivityPower = 0.01; // Ignore tails 20 dB below recent input.
constexpr double kQuietStateThreshold = 1.0e-24;

} // namespace

void AdaptiveCompressorGain::Prepare(const DspProcessingContext& context)
{
    referencePower_.resize(context.maximumFrameCount);
    gain_.Prepare(context);
    PrepareWeighting(context.sampleRate);
    intervalSamples_ = static_cast<std::size_t>(std::round(context.sampleRate * kMeasurementIntervalSeconds));
    const auto intervalSeconds = static_cast<double>(intervalSamples_) / context.sampleRate;
    powerCoefficient_ = -std::expm1(-intervalSeconds / kPowerAverageSeconds);
    correctionCoefficient_ = -std::expm1(-intervalSeconds / kCorrectionSeconds);
    warmupIntervals_ = static_cast<std::size_t>(std::ceil(kWarmupSeconds / intervalSeconds));
    Reset(1.0);
}

void AdaptiveCompressorGain::PrepareWeighting(double sampleRate) noexcept
{
    // BS.1770 K weighting, bilinear coefficients at the actual sample rate.
    const auto shelfTangent = std::tan(std::numbers::pi * 1681.974450955533 / sampleRate);
    constexpr double kShelfQ = 0.7071752369554196;
    const auto highGain = DbToGain(3.999843853973347);
    const auto bandGain = std::pow(highGain, 0.4996667741545416);
    const auto shelfSquare = shelfTangent * shelfTangent;
    const auto shelfDenominator = 1.0 + shelfTangent / kShelfQ + shelfSquare;
    shelf_ = {
        (highGain + bandGain * shelfTangent / kShelfQ + shelfSquare) / shelfDenominator,
        2.0 * (shelfSquare - highGain) / shelfDenominator,
        (highGain - bandGain * shelfTangent / kShelfQ + shelfSquare) / shelfDenominator,
        2.0 * (shelfSquare - 1.0) / shelfDenominator,
        (1.0 - shelfTangent / kShelfQ + shelfSquare) / shelfDenominator
    };
    const auto highPassTangent = std::tan(std::numbers::pi * 38.13547087602444 / sampleRate);
    constexpr double kHighPassQ = 0.5003270373238773;
    const auto highPassSquare = highPassTangent * highPassTangent;
    const auto highPassDenominator = 1.0 + highPassTangent / kHighPassQ + highPassSquare;
    highPass_ = {
        1.0, -2.0, 1.0,
        2.0 * (highPassSquare - 1.0) / highPassDenominator,
        (1.0 - highPassTangent / kHighPassQ + highPassSquare) / highPassDenominator
    };
}

double AdaptiveCompressorGain::FilterState::Process(double input, const Coefficients& coefficients) noexcept
{
    const auto output = coefficients.b0 * input + z1;
    z1 = coefficients.b1 * input - coefficients.a1 * output + z2;
    z2 = coefficients.b2 * input - coefficients.a2 * output;
    return output;
}

double AdaptiveCompressorGain::WeightedPower(double input, ChannelState& state) const noexcept
{
    const auto weighted = state.highPass.Process(state.shelf.Process(input, shelf_), highPass_);
    return weighted * weighted;
}

void AdaptiveCompressorGain::FilterState::ClearQuietState() noexcept
{
    z1 = std::abs(z1) < kQuietStateThreshold ? 0.0 : z1;
    z2 = std::abs(z2) < kQuietStateThreshold ? 0.0 : z2;
}

void AdaptiveCompressorGain::Reset(double outputGain) noexcept
{
    const auto initialGain = enabled_ ? std::clamp(outputGain,
        DbToGain(kCompressorOutputMinimumDb), DbToGain(kCompressorOutputMaximumDb)) : outputGain;
    correctionDb_ = enabled_ ? 20.0 * std::log10(initialGain) : 0.0;
    gain_.SetTarget(initialGain);
    gain_.Reset();
    ResetMeasurement();
}

double AdaptiveCompressorGain::OutputDb() const noexcept
{
    return 20.0 * std::log10(gain_.CurrentValue());
}

void AdaptiveCompressorGain::SetControls(bool enabled, double outputGain) noexcept
{
    if (enabled != enabled_)
    {
        enabled_ = enabled;
        ResetMeasurement();
        if (enabled_)
        {
            const auto initialGain = std::clamp(outputGain,
                DbToGain(kCompressorOutputMinimumDb), DbToGain(kCompressorOutputMaximumDb));
            correctionDb_ = 20.0 * std::log10(initialGain);
            gain_.SetTarget(initialGain);
        }
    }
    if (!enabled_)
    {
        gain_.SetTarget(outputGain);
    }
}

void AdaptiveCompressorGain::ResetMeasurement() noexcept
{
    referenceStates_ = {};
    wetStates_ = {};
    accumulatedSamples_ = 0;
    activeIntervals_ = 0;
    referenceSum_ = 0.0;
    wetSum_ = 0.0;
    referenceMeanPower_ = 0.0;
    wetMeanPower_ = 0.0;
}

void AdaptiveCompressorGain::CaptureReference(DspAudioBuffer audio, DspChannelMode mode) noexcept
{
    if (!enabled_)
    {
        return;
    }
    const auto selected = SelectChannels(audio, mode);
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        auto power = 0.0;
        if (selected.left != nullptr)
        {
            power += WeightedPower(selected.left[index], referenceStates_[0]);
        }
        if (selected.right != nullptr)
        {
            power += WeightedPower(selected.right[index], referenceStates_[1]);
        }
        referencePower_[index] = power;
    }
}

void AdaptiveCompressorGain::Process(DspAudioBuffer audio, DspChannelMode mode) noexcept
{
    const auto selected = SelectChannels(audio, mode);
    if (!enabled_)
    {
        for (std::size_t index = 0; index < audio.frameCount; ++index)
        {
            selected.ApplyGain(index, gain_.NextValue());
        }
        return;
    }
    const auto inverseChannelCount = 1.0 / static_cast<double>(
        std::max(selected.channelCount, std::size_t{ 1 }));
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        auto wetPower = 0.0;
        if (selected.left != nullptr)
        {
            wetPower += WeightedPower(selected.left[index], wetStates_[0]);
        }
        if (selected.right != nullptr)
        {
            wetPower += WeightedPower(selected.right[index], wetStates_[1]);
        }
        referenceSum_ += referencePower_[index] * inverseChannelCount;
        wetSum_ += wetPower * inverseChannelCount;
        if (++accumulatedSamples_ == intervalSamples_)
        {
            FinishMeasurementInterval();
        }
        selected.ApplyGain(index, gain_.NextValue());
    }
}

void AdaptiveCompressorGain::FinishMeasurementInterval() noexcept
{
    const auto referencePower = referenceSum_ / static_cast<double>(intervalSamples_);
    const auto wetPower = wetSum_ / static_cast<double>(intervalSamples_);
    referenceMeanPower_ += powerCoefficient_ * (referencePower - referenceMeanPower_);
    wetMeanPower_ += powerCoefficient_ * (wetPower - wetMeanPower_);
    // Clear inaudible meter tails at control rate instead of adding checks to every sample.
    for (auto* states : { &referenceStates_, &wetStates_ })
    {
        for (auto& state : *states)
        {
            state.shelf.ClearQuietState();
            state.highPass.ClearQuietState();
        }
    }
    referenceMeanPower_ = referenceMeanPower_ < kQuietStateThreshold ? 0.0 : referenceMeanPower_;
    wetMeanPower_ = wetMeanPower_ < kQuietStateThreshold ? 0.0 : wetMeanPower_;
    referenceSum_ = 0.0;
    wetSum_ = 0.0;
    accumulatedSamples_ = 0;
    const auto activityFloor = std::max(kMinimumReferencePower, referenceMeanPower_ * kRelativeActivityPower);
    if (referencePower < activityFloor || wetMeanPower_ <= 0.0)
    {
        // Freeze both the correction and an unfinished ramp during silence/tails.
        gain_.SetTarget(gain_.CurrentValue());
        return;
    }
    if (activeIntervals_ < warmupIntervals_)
    {
        ++activeIntervals_;
        return;
    }
    // Metered wet excludes Output: auto writes an absolute gain, without a second hidden correction.
    const auto targetDb = std::clamp(
        10.0 * std::log10(referenceMeanPower_ / wetMeanPower_),
        kCompressorOutputMinimumDb, kCompressorOutputMaximumDb);
    if (std::abs(targetDb - correctionDb_) > kDeadbandDb)
    {
        correctionDb_ += correctionCoefficient_ * (targetDb - correctionDb_);
        gain_.SetTarget(DbToGain(correctionDb_));
    }
}

} // namespace consolidator::max::dsp::processors
