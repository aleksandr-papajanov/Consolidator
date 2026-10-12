#include "Cla2aProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>

#include "../../DspMath.h"
#include "../../DspProcessingPrimitives.h"

namespace consolidator::max::dsp::processors
{

void Cla2aProcessor::Prepare(const DspProcessingContext& context) noexcept
{
    reduction_.Prepare(context);
    output_.Prepare(context);
    outputCalibration_ = DbToGain(kConfig.outputCalibrationDb);
    PrepareDetector(context.sampleRate);
    PrepareColourFilters(context);
    Reset();
}

void Cla2aProcessor::Reset() noexcept
{
    reduction_.Reset();
    cachedReduction_ = -1.0;
    UpdateReductionCurve(reduction_.CurrentValue());
    output_.Reset();
    detectorPower_ = 0.0;
    fastReductionDb_ = 0.0;
    slowReductionDb_ = 0.0;
    highPassStates_ = {};
    highShelfFilter_.Reset();
}

void Cla2aProcessor::SetControls(double reduction, double outputGain) noexcept
{
    reduction_.SetTarget(reduction);
    output_.SetTarget(outputGain);
}

void Cla2aProcessor::Process(DspAudioBuffer audio, DspChannelMode mode) noexcept
{
    const auto selected = SelectChannels(audio, mode);
    const std::array<double*, 2> channels{ selected.left, selected.right };
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        const auto reduction = reduction_.NextValue();
        UpdateReductionCurve(reduction);
        // The curve reads the preceding power state; this sample then charges the detector.
        const auto targetDb = reduction > 0.0 ? CalculateTargetReductionDb() : 0.0;
        UpdateDetector(channels, index);
        const auto compressionGain = UpdateOpticalGain(targetDb);
        for (std::size_t channel = 0; channel < channels.size(); ++channel)
        {
            if (channels[channel] != nullptr)
            {
                channels[channel][index] = ProcessHighPassSample(
                    channels[channel][index] * compressionGain, channel);
            }
        }
    }
    highShelfFilter_.Process(audio, mode);
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        selected.ApplyGain(index, output_.NextValue() * outputCalibration_);
    }
}

void Cla2aProcessor::PrepareDetector(double sampleRate) noexcept
{
    detectorAttackCoefficient_ = TimeCoefficient(kConfig.detectorAttackSeconds, sampleRate);
    detectorReleaseCoefficient_ = TimeCoefficient(kConfig.detectorReleaseSeconds, sampleRate);
    inverseSampleRate_ = 1.0 / sampleRate;
}

void Cla2aProcessor::UpdateReductionCurve(double reduction) noexcept
{
    if (reduction == cachedReduction_)
    {
        return;
    }
    // Preserve the identified reference taper; only the UI is now a normalized amount.
    constexpr double kPivot = 0.4;
    const auto displayed = reduction <= kPivot
        ? 40.0 * std::expm1(-5.994 * reduction) / std::expm1(-5.994 * kPivot)
        : 40.0 + 60.0 * std::expm1(4.6213 * (reduction - kPivot)) / std::expm1(4.6213 * (1.0 - kPivot));
    thresholdDb_ = kConfig.thresholdAt40Db + kConfig.lowReductionSlopeDbPerUnit *
        std::max(40.0 - displayed, 0.0);
    for (std::size_t region = 0; region < kConfig.thresholdSlopesDbPerUnit.size(); ++region)
    {
        const auto width = std::clamp(displayed - 40.0 - static_cast<double>(region) * 10.0, 0.0, 10.0);
        thresholdDb_ -= width * kConfig.thresholdSlopesDbPerUnit[region];
    }
    const auto start = DbToGain(thresholdDb_ - kConfig.kneeDb * 0.5);
    compressionStartPower_ = start * start;
    cachedReduction_ = reduction;
}

void Cla2aProcessor::PrepareColourFilters(const DspProcessingContext& context) noexcept
{
    const auto warpedFrequency = std::tan(std::numbers::pi *
        std::min(kConfig.highPassHz, context.sampleRate * 0.499) / context.sampleRate);
    highPassFeedforward_ = 1.0 / (1.0 + warpedFrequency);
    highPassFeedback_ = (1.0 - warpedFrequency) * highPassFeedforward_;
    highShelfFilter_ = {};
    highShelfFilter_.Prepare(context);
    highShelfFilter_.SetParameters({ 1, static_cast<std::uint32_t>(DspFilterType::HighShelf),
        static_cast<float>(kConfig.highShelfHz), static_cast<float>(kConfig.highShelfDb),
        0.0F, static_cast<float>(kConfig.highShelfQ) }, true);
}

double Cla2aProcessor::CalculateTargetReductionDb() const noexcept
{
    auto targetDb = 0.0;
    if (detectorPower_ > compressionStartPower_)
    {
        const auto overDb = 10.0 * std::log10(detectorPower_) - thresholdDb_;
        const auto halfKnee = kConfig.kneeDb * 0.5;
        if (overDb >= halfKnee)
        {
            targetDb = kConfig.compressionSlope * overDb;
        }
        else
        {
            const auto position = overDb + halfKnee;
            targetDb = kConfig.compressionSlope * position * position / (2.0 * kConfig.kneeDb);
        }
    }
    // Deep reduction flattens the measured transfer slope; this changes compression, not audio colour.
    const auto excess = std::max(targetDb - kConfig.deepReductionStartDb, 0.0);
    return targetDb - kConfig.deepReductionStrength * excess * excess /
        (excess + kConfig.deepReductionSpanDb);
}

void Cla2aProcessor::UpdateDetector(
    const std::array<double*, 2>& channels, std::size_t index) noexcept
{
    auto amplitude = 0.0;
    for (std::size_t channel = 0; channel < channels.size(); ++channel)
    {
        if (channels[channel] != nullptr)
        {
            const auto sample = channels[channel][index];
            amplitude = std::max(amplitude, std::abs(sample));
        }
    }
    const auto power = amplitude * amplitude;
    const auto detectorCoefficient = power > detectorPower_
        ? detectorAttackCoefficient_ : detectorReleaseCoefficient_;
    detectorPower_ += detectorCoefficient * (power - detectorPower_);
}

double Cla2aProcessor::UpdateOpticalGain(double targetDb) noexcept
{
    const auto chargeDrive = std::sqrt((targetDb + 0.1) / (targetDb + kConfig.chargeSaturationDb));
    const auto deepChargePosition = targetDb / kConfig.fastChargeFlattenDepthDb;
    const auto fastChargeDrive = chargeDrive * (1.0 + targetDb / kConfig.fastChargeBoostDepthDb) /
        (1.0 + deepChargePosition * deepChargePosition);
    const auto recoveryDrive = 1.0 + targetDb / kConfig.recoveryTargetScaleDb;
    const auto fastRate = targetDb >= fastReductionDb_
        ? fastChargeDrive / kConfig.fastChargeScale
        : (1.0 + fastReductionDb_ / kConfig.recoveryDepthDb) * recoveryDrive / kConfig.fastRecoverySeconds;
    const auto slowRate = targetDb >= slowReductionDb_
        ? chargeDrive / kConfig.slowChargeScale
        : (1.0 + slowReductionDb_ / kConfig.recoveryDepthDb) * recoveryDrive * recoveryDrive /
            kConfig.slowRecoverySeconds;
    // Backward Euler keeps both memories between their previous state and target at any rate.
    const auto fastStep = inverseSampleRate_ * fastRate;
    const auto slowStep = inverseSampleRate_ * slowRate;
    const auto fastCoefficient = fastStep / (1.0 + fastStep);
    const auto slowCoefficient = slowStep / (1.0 + slowStep);
    fastReductionDb_ += fastCoefficient * (targetDb - fastReductionDb_);
    slowReductionDb_ += slowCoefficient * (targetDb - slowReductionDb_);
    const auto reductionDb = std::lerp(fastReductionDb_, slowReductionDb_, kConfig.slowFraction);
    const auto compressionGain = reductionDb == 0.0 ? 1.0 : DbToGain(-reductionDb);
    return compressionGain;
}

double Cla2aProcessor::ProcessHighPassSample(double input, std::size_t channel) noexcept
{
    auto& state = highPassStates_[channel];
    const auto output = highPassFeedforward_ * (input - state[0]) + highPassFeedback_ * state[1];
    state[0] = input;
    state[1] = output;
    return output;
}

} // namespace consolidator::max::dsp::processors
