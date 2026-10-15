#include "Cla3aProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>

#include "../../Primitives/Math.h"
#include "../../Primitives/Filters/OnePole.h"
#include "../../Primitives/ChannelView.h"

namespace consolidator::audio::algorithms
{

Cla3aProcessor::Cla3aProcessor(CompressorControlMode controlMode) noexcept
    : controlMode_(controlMode),
      peakReduction_(controlMode == CompressorControlMode::Emulation
          ? Cla3aEmulationControls{}.peakReduction : Cla3aControls{}.reduction),
      gain_(controlMode == CompressorControlMode::Emulation
          ? Cla3aEmulationControls{}.gain : Cla3aControls{}.outputGain)
{
}

void Cla3aProcessor::Prepare(const ProcessingContext& context) noexcept
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

void Cla3aProcessor::Reset() noexcept
{
    peakReduction_.Reset();
    cachedPeakReduction_ = -1.0;
    UpdateReductionCurve(peakReduction_.CurrentValue());
    gain_.Reset();
    cachedGain_ = -1.0;
    UpdateOutputGain(gain_.CurrentValue());
    reductionDb_ = 0.0;
    detectorPower_ = 0.0;
    detectorShelfStates_ = {};
    fastReductionDb_ = 0.0;
    slowReductionDb_ = 0.0;
    if (controlMode_ == CompressorControlMode::Emulation)
    {
        highPassStates_ = {};
        highShelfFilter_.Reset();
    }
}

void Cla3aProcessor::SetControls(const Cla3aControls& controls) noexcept
{
    const auto emulation = controlMode_ == CompressorControlMode::Emulation;
    peakReduction_.SetTarget(emulation ? controls.emulation.peakReduction : controls.reduction);
    gain_.SetTarget(emulation ? controls.emulation.gain : controls.outputGain);
}

void Cla3aProcessor::Process(AudioBuffer audio, DetectorBuffer detector, ChannelMode mode) noexcept
{
    reductionDb_ = 0.0;
    const auto selected = primitives::SelectChannels(audio, mode);
    const std::array<const double*, 2> detectorChannels{
        mode == ChannelMode::Right ? nullptr : detector.left,
        mode == ChannelMode::Left ? nullptr : detector.right };
    const std::array<double*, 2> channels{ selected.left, selected.right };
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        const auto peakReduction = peakReduction_.NextValue();
        UpdateReductionCurve(peakReduction);
        // The curve reads the preceding power state; this sample then charges the detector.
        const auto targetDb = peakReduction > 0.0 ? CalculateTargetReductionDb() : 0.0;
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
                    channels[channel][index] = highPassStates_[channel].Process(
                        channels[channel][index],
                        highPassCoefficients_);
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

void Cla3aProcessor::PrepareDetector(double sampleRate) noexcept
{
    detectorAttackCoefficient_ = primitives::TimeCoefficient(kConfig.detectorAttackSeconds, sampleRate);
    detectorReleaseCoefficient_ = primitives::TimeCoefficient(kConfig.detectorReleaseSeconds, sampleRate);
    fastAttackCoefficient_ = primitives::TimeCoefficient(kConfig.fastAttackSeconds, sampleRate);
    slowAttackCoefficient_ = primitives::TimeCoefficient(kConfig.slowAttackSeconds, sampleRate);
    inverseSampleRate_ = 1.0 / sampleRate;
    detectorShelfCoefficient_ = primitives::filters::onepole::FilterCoefficient(kConfig.detectorShelfHz, sampleRate);
    detectorLowLoss_ = 1.0 - primitives::DbToGain(kConfig.detectorLowDb);
    fastZeroTargetCoefficient_ = inverseSampleRate_ /
        (inverseSampleRate_ + kConfig.releaseFloorSeconds + kConfig.fastReleaseSeconds);
    slowZeroTargetCoefficient_ = inverseSampleRate_ /
        (inverseSampleRate_ + kConfig.releaseFloorSeconds + kConfig.slowReleaseSeconds);
}

void Cla3aProcessor::UpdateReductionCurve(double peakReduction) noexcept
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
        const auto position = (peakReduction - 0.6) / 0.4;
        const auto& law = kConfig.thresholdLaw;
        thresholdDb_ = law[0] + position * (law[1] + position * (law[2] + position * law[3]));
    }
    const auto start = primitives::DbToGain(thresholdDb_ - kConfig.kneeDb * 0.5);
    compressionStartPower_ = start * start;
    makeupGain_ = controlMode_ == CompressorControlMode::Normal
        ? CalculateMakeupGain(peakReduction) : 1.0;
    cachedPeakReduction_ = peakReduction;
}

void Cla3aProcessor::UpdateOutputGain(double gain) noexcept
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
        outputGain_ = primitives::DbToGain((gain - kConfig.gainUnityPosition) * kConfig.gainSpanDb);
    }
    cachedGain_ = gain;
}

void Cla3aProcessor::PrepareColourFilters(const ProcessingContext& context) noexcept
{
    highPassCoefficients_ = primitives::filters::DesignHighPass(
        std::min(kConfig.highPassHz, context.sampleRate * 0.499),
        kConfig.highPassQ,
        context.sampleRate);
    highShelfFilter_ = {};
    highShelfFilter_.Prepare(context);
    highShelfFilter_.SetParameters({ 1, static_cast<std::uint32_t>(FilterType::HighShelf),
        static_cast<float>(kConfig.highShelfHz), static_cast<float>(kConfig.highShelfDb),
        0.0F, static_cast<float>(kConfig.highShelfQ) }, true);
}

double Cla3aProcessor::CalculateTargetReductionDb() const noexcept
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
    return targetDb;
}

void Cla3aProcessor::UpdateDetector(
    const std::array<const double*, 2>& channels, std::size_t index) noexcept
{
    auto amplitude = 0.0;
    for (std::size_t channel = 0; channel < channels.size(); ++channel)
    {
        if (channels[channel] != nullptr)
        {
            const auto sample = channels[channel][index];
            const auto high = primitives::filters::onepole::HighPass(
                sample, detectorShelfCoefficient_, detectorShelfStates_[channel]);
            const auto detectorSample = sample - detectorLowLoss_ * (sample - high);
            amplitude = std::max(amplitude, std::abs(detectorSample));
        }
    }
    const auto power = amplitude * amplitude;
    const auto detectorCoefficient = power > detectorPower_
        ? detectorAttackCoefficient_ : detectorReleaseCoefficient_;
    detectorPower_ += detectorCoefficient * (power - detectorPower_);
}

double Cla3aProcessor::UpdateOpticalGain(double targetDb) noexcept
{
    // Strong remaining reduction shortens recovery; zero target retains the slower optical tail.
    auto fastCoefficient = fastAttackCoefficient_;
    auto slowCoefficient = slowAttackCoefficient_;
    if (targetDb < fastReductionDb_ || targetDb < slowReductionDb_)
    {
        if (targetDb == 0.0)
        {
            fastCoefficient = fastZeroTargetCoefficient_;
            slowCoefficient = slowZeroTargetCoefficient_;
        }
        else
        {
            const auto releaseDrive = std::exp(-targetDb / kConfig.releaseDriveDb);
            if (targetDb < fastReductionDb_)
            {
                const auto seconds = kConfig.releaseFloorSeconds + kConfig.fastReleaseSeconds * releaseDrive;
                fastCoefficient = inverseSampleRate_ / (inverseSampleRate_ + seconds);
            }
            if (targetDb < slowReductionDb_)
            {
                const auto seconds = kConfig.releaseFloorSeconds + kConfig.slowReleaseSeconds * releaseDrive;
                slowCoefficient = inverseSampleRate_ / (inverseSampleRate_ + seconds);
            }
        }
    }
    fastReductionDb_ += fastCoefficient * (targetDb - fastReductionDb_);
    slowReductionDb_ += slowCoefficient * (targetDb - slowReductionDb_);
    const auto reductionDb = std::lerp(fastReductionDb_, slowReductionDb_, kConfig.slowFraction);
    reductionDb_ = std::max(reductionDb_, reductionDb);
    const auto compressionGain = reductionDb == 0.0 ? 1.0 : primitives::DbToGain(-reductionDb);
    return compressionGain;
}

double Cla3aProcessor::ReductionDb() const noexcept
{
    return reductionDb_;
}

// Model-specific settings compensation; independent of live detector history.
double Cla3aProcessor::CalculateMakeupGain(double reduction) noexcept
{
    const auto position = std::max(reduction - 0.397, 0.0);
    return primitives::DbToGain(37.078 * position * position / (position + 0.198));
}

} // namespace consolidator::audio::algorithms
