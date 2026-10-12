#include "Cla3aProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>

#include "../../DspMath.h"
#include "../../DspOnePole.h"
#include "../../DspProcessingPrimitives.h"

namespace consolidator::max::dsp::processors
{

void Cla3aProcessor::Prepare(const DspProcessingContext& context) noexcept
{
    peakReduction_.Prepare(context);
    gain_.Prepare(context);
    PrepareDetector(context.sampleRate);
    PrepareColourFilters(context);
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
    detectorPower_ = 0.0;
    detectorShelfStates_ = {};
    fastReductionDb_ = 0.0;
    slowReductionDb_ = 0.0;
    highPassStates_ = {};
    highShelfFilter_.Reset();
}

void Cla3aProcessor::SetControls(double gain, double peakReduction) noexcept
{
    peakReduction_.SetTarget(peakReduction);
    gain_.SetTarget(gain);
}

void Cla3aProcessor::Process(DspAudioBuffer audio, DspChannelMode mode) noexcept
{
    const auto selected = SelectChannels(audio, mode);
    const std::array<double*, 2> channels{ selected.left, selected.right };
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        const auto peakReduction = peakReduction_.NextValue();
        UpdateReductionCurve(peakReduction);
        // The curve reads the preceding power state; this sample then charges the detector.
        const auto targetDb = peakReduction > 0.0 ? CalculateTargetReductionDb() : 0.0;
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
        UpdateOutputGain(gain_.NextValue());
        selected.ApplyGain(index, outputGain_);
    }
}

void Cla3aProcessor::PrepareDetector(double sampleRate) noexcept
{
    detectorAttackCoefficient_ = TimeCoefficient(kConfig.detectorAttackSeconds, sampleRate);
    detectorReleaseCoefficient_ = TimeCoefficient(kConfig.detectorReleaseSeconds, sampleRate);
    fastAttackCoefficient_ = TimeCoefficient(kConfig.fastAttackSeconds, sampleRate);
    slowAttackCoefficient_ = TimeCoefficient(kConfig.slowAttackSeconds, sampleRate);
    inverseSampleRate_ = 1.0 / sampleRate;
    detectorShelfCoefficient_ = onepole::FilterCoefficient(kConfig.detectorShelfHz, sampleRate);
    detectorLowLoss_ = 1.0 - DbToGain(kConfig.detectorLowDb);
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
    const auto position = (peakReduction - 0.6) / 0.4;
    const auto& law = kConfig.thresholdLaw;
    thresholdDb_ = law[0] + position * (law[1] + position * (law[2] + position * law[3]));
    const auto start = DbToGain(thresholdDb_ - kConfig.kneeDb * 0.5);
    compressionStartPower_ = start * start;
    cachedPeakReduction_ = peakReduction;
}

void Cla3aProcessor::UpdateOutputGain(double gain) noexcept
{
    if (gain == cachedGain_)
    {
        return;
    }
    outputGain_ = DbToGain((gain - kConfig.gainUnityPosition) * kConfig.gainSpanDb);
    cachedGain_ = gain;
}

void Cla3aProcessor::PrepareColourFilters(const DspProcessingContext& context) noexcept
{
    const auto omega = 2.0 * std::numbers::pi *
        std::min(kConfig.highPassHz, context.sampleRate * 0.499) / context.sampleRate;
    const auto cosine = std::cos(omega);
    const auto alpha = std::sin(omega) / (2.0 * kConfig.highPassQ);
    const auto denominator = 1.0 + alpha;
    const auto numerator = (1.0 + cosine) / (2.0 * denominator);
    highPassCoefficients_ = { numerator, -2.0 * numerator, numerator,
        -2.0 * cosine / denominator, (1.0 - alpha) / denominator };
    highShelfFilter_ = {};
    highShelfFilter_.Prepare(context);
    highShelfFilter_.SetParameters({ 1, static_cast<std::uint32_t>(DspFilterType::HighShelf),
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
    const std::array<double*, 2>& channels, std::size_t index) noexcept
{
    auto amplitude = 0.0;
    for (std::size_t channel = 0; channel < channels.size(); ++channel)
    {
        if (channels[channel] != nullptr)
        {
            const auto sample = channels[channel][index];
            const auto high = onepole::HighPass(
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
    const auto compressionGain = reductionDb == 0.0 ? 1.0 : DbToGain(-reductionDb);
    return compressionGain;
}

double Cla3aProcessor::ProcessHighPassSample(double input, std::size_t channel) noexcept
{
    const auto& c = highPassCoefficients_;
    auto& state = highPassStates_[channel];
    const auto output = c[0] * input + state[0];
    state[0] = c[1] * input - c[3] * output + state[1];
    state[1] = c[2] * input - c[4] * output;
    return output;
}

} // namespace consolidator::max::dsp::processors
