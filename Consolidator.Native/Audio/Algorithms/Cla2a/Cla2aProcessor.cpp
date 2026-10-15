#include "Cla2aProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "../../Primitives/Math.h"
#include "../../Primitives/ChannelView.h"

namespace consolidator::audio::algorithms
{

double Cla2aControlScale::ToDisplayValue(double normalized) const noexcept
{
    const auto pivotValue = pivot * 100.0;
    if (normalized <= pivot)
    {
        return pivotValue * std::expm1(-lowExponent * normalized) / std::expm1(-lowExponent * pivot);
    }
    return pivotValue + (100.0 - pivotValue) * std::expm1(highExponent * (normalized - pivot)) /
        std::expm1(highExponent * (1.0 - pivot));
}

double Cla2aControlScale::ToNormalizedValue(double displayed) const noexcept
{
    const auto value = std::clamp(displayed, 0.0, 100.0);
    const auto pivotValue = pivot * 100.0;
    if (value <= pivotValue)
    {
        return -std::log1p(value / pivotValue * std::expm1(-lowExponent * pivot)) / lowExponent;
    }
    return pivot + std::log1p((value - pivotValue) / (100.0 - pivotValue) *
        std::expm1(highExponent * (1.0 - pivot))) / highExponent;
}

Cla2aProcessor::Cla2aProcessor(CompressorControlMode controlMode) noexcept
    : controlMode_(controlMode),
      peakReduction_(controlMode == CompressorControlMode::Emulation
          ? Cla2aEmulationControls{}.peakReduction : Cla2aControls{}.reduction),
      gain_(controlMode == CompressorControlMode::Emulation
          ? Cla2aEmulationControls{}.gain : Cla2aControls{}.outputGain)
{
}

void Cla2aProcessor::Prepare(const ProcessingContext& context) noexcept
{
    peakReduction_.Prepare(context);
    gain_.Prepare(context);
    PrepareDetector(context.sampleRate);
    if (controlMode_ == CompressorControlMode::Emulation)
    {
        PrepareColourFilters(context);
    }
    Reset();
}

void Cla2aProcessor::Reset() noexcept
{
    peakReduction_.Reset();
    cachedPeakReduction_ = -1.0;
    UpdateReductionCurve(peakReduction_.CurrentValue());
    gain_.Reset();
    cachedGain_ = -1.0;
    UpdateOutputGain(gain_.CurrentValue());
    reductionDb_ = 0.0;
    detectorPower_ = 0.0;
    fastReductionDb_ = 0.0;
    slowReductionDb_ = 0.0;
    if (controlMode_ == CompressorControlMode::Emulation)
    {
        for (auto& filter : highPassFilters_)
        {
            filter.Reset();
        }
        highShelfFilter_.Reset();
    }
}

void Cla2aProcessor::SetControls(const Cla2aControls& controls) noexcept
{
    const auto emulation = controlMode_ == CompressorControlMode::Emulation;
    peakReduction_.SetTarget(emulation ? controls.emulation.peakReduction : controls.reduction);
    gain_.SetTarget(emulation ? controls.emulation.gain : controls.outputGain);
}

void Cla2aProcessor::Process(AudioBuffer audio, DetectorBuffer detector, ChannelMode mode) noexcept
{
    reductionDb_ = 0.0;
    const auto selected = primitives::SelectChannels(audio, mode);
    const std::array<const double*, 2> detectorChannels{
        mode == ChannelMode::Right ? nullptr : detector.left,
        mode == ChannelMode::Left ? nullptr : detector.right };
    const std::array<double*, 2> channels{ selected.left, selected.right };
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        const auto reduction = peakReduction_.NextValue();
        UpdateReductionCurve(reduction);
        // The curve reads the preceding power state; this sample then charges the detector.
        const auto targetDb = reduction > 0.0 ? CalculateTargetReductionDb() : 0.0;
        UpdateDetector(detectorChannels, index);
        const auto compressionGain = UpdateOpticalGain(targetDb) * makeupGain_;
        for (std::size_t channel = 0; channel < channels.size(); ++channel)
        {
            if (channels[channel] != nullptr)
            {
                channels[channel][index] *= compressionGain;
            }
        }
    }
    if (controlMode_ == CompressorControlMode::Emulation)
    {
        for (std::size_t channel = 0; channel < channels.size(); ++channel)
        {
            if (channels[channel] != nullptr)
            {
                for (std::size_t index = 0; index < audio.frameCount; ++index)
                {
                    channels[channel][index] = highPassFilters_[channel].Process(
                        channels[channel][index]);
                }
            }
        }
        highShelfFilter_.Process(audio, mode);
    }
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        UpdateOutputGain(gain_.NextValue());
        selected.ApplyGain(index, outputGain_);
    }
}

void Cla2aProcessor::PrepareDetector(double sampleRate) noexcept
{
    detectorAttackCoefficient_ = primitives::TimeCoefficient(kConfig.detectorAttackSeconds, sampleRate);
    detectorReleaseCoefficient_ = primitives::TimeCoefficient(kConfig.detectorReleaseSeconds, sampleRate);
    inverseSampleRate_ = 1.0 / sampleRate;
}

void Cla2aProcessor::UpdateReductionCurve(double peakReduction) noexcept
{
    if (peakReduction == cachedPeakReduction_)
    {
        return;
    }
    if (controlMode_ == CompressorControlMode::Normal)
    {
        thresholdDb_ = kNormalThresholdMaximumDb - kNormalThresholdSpanDb * peakReduction;
    }
    else
    {
        const auto displayed = kCla2aPeakReductionScale.ToDisplayValue(peakReduction);
        thresholdDb_ = kConfig.thresholdAt40Db + kConfig.lowReductionSlopeDbPerUnit *
            std::max(40.0 - displayed, 0.0);
        for (std::size_t region = 0; region < kConfig.thresholdSlopesDbPerUnit.size(); ++region)
        {
            const auto width = std::clamp(displayed - 40.0 - static_cast<double>(region) * 10.0, 0.0, 10.0);
            thresholdDb_ -= width * kConfig.thresholdSlopesDbPerUnit[region];
        }
    }
    const auto start = primitives::DbToGain(thresholdDb_ - kConfig.kneeDb * 0.5);
    compressionStartPower_ = start * start;
    makeupGain_ = controlMode_ == CompressorControlMode::Normal
        ? CalculateMakeupGain(peakReduction) : 1.0;
    cachedPeakReduction_ = peakReduction;
}

void Cla2aProcessor::UpdateOutputGain(double gain) noexcept
{
    if (gain == cachedGain_)
    {
        return;
    }
    if (controlMode_ == CompressorControlMode::Normal)
    {
        outputGain_ = gain;
    }
    else
    {
        const auto displayed = kCla2aGainScale.ToDisplayValue(gain);
        auto gainDb = kConfig.gainFloorDb;
        for (std::size_t region = 0; region < kConfig.gainSlopesDbPerUnit.size(); ++region)
        {
            const auto width = std::clamp(displayed - static_cast<double>(region) * 10.0, 0.0, 10.0);
            gainDb += width * kConfig.gainSlopesDbPerUnit[region];
        }
        outputGain_ = primitives::DbToGain(gainDb);
    }
    cachedGain_ = gain;
}

void Cla2aProcessor::PrepareColourFilters(const ProcessingContext& context) noexcept
{
    for (auto& filter : highPassFilters_)
    {
        filter.Prepare(kConfig.highPassHz, context.sampleRate);
    }
    highShelfFilter_ = {};
    highShelfFilter_.Prepare(context);
    highShelfFilter_.SetParameters({ 1, static_cast<std::uint32_t>(FilterType::HighShelf),
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
    const std::array<const double*, 2>& channels, std::size_t index) noexcept
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
    reductionDb_ = std::max(reductionDb_, reductionDb);
    const auto compressionGain = reductionDb == 0.0 ? 1.0 : primitives::DbToGain(-reductionDb);
    return compressionGain;
}

double Cla2aProcessor::ReductionDb() const noexcept
{
    return reductionDb_;
}

// Model-specific settings compensation; independent of live detector history.
double Cla2aProcessor::CalculateMakeupGain(double reduction) noexcept
{
    const auto position = std::max(reduction - 0.413, 0.0);
    return primitives::DbToGain(36.471 * position * position / (position + 0.127));
}

} // namespace consolidator::audio::algorithms
