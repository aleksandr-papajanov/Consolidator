#include "Cla3aProcessor.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <numbers>

#include "DspMath.h"
#include "DspOnePole.h"
#include "DspProcessingPrimitives.h"

namespace consolidator::max::dsp
{

void Cla3aProcessor::Prepare(const DspProcessingContext& context) noexcept
{
    input_.Prepare(context);
    output_.Prepare(context);
    inputCalibration_ = DbToGain(tuning_.inputCalibrationDb);
    outputCalibration_ = DbToGain(tuning_.outputCalibrationDb);
    const auto start = DbToGain(tuning_.thresholdDb - tuning_.kneeDb * 0.5);
    compressionStartPower_ = start * start;
    detectorAttackCoefficient_ = TimeCoefficient(tuning_.detectorAttackSeconds, context.sampleRate);
    detectorReleaseCoefficient_ = TimeCoefficient(tuning_.detectorReleaseSeconds, context.sampleRate);
    fastAttackCoefficient_ = TimeCoefficient(tuning_.fastAttackSeconds, context.sampleRate);
    slowAttackCoefficient_ = TimeCoefficient(tuning_.slowAttackSeconds, context.sampleRate);
    inverseSampleRate_ = 1.0 / context.sampleRate;
    detectorShelfCoefficient_ = onepole::FilterCoefficient(tuning_.detectorShelfHz, context.sampleRate);
    detectorLowLoss_ = 1.0 - DbToGain(tuning_.detectorLowDb);
    fastZeroTargetCoefficient_ = inverseSampleRate_ /
        (inverseSampleRate_ + tuning_.releaseFloorSeconds + tuning_.fastReleaseSeconds);
    slowZeroTargetCoefficient_ = inverseSampleRate_ /
        (inverseSampleRate_ + tuning_.releaseFloorSeconds + tuning_.slowReleaseSeconds);
    const auto omega = 2.0 * std::numbers::pi *
        std::min(tuning_.highPassHz, context.sampleRate * 0.499) / context.sampleRate;
    const auto cosine = std::cos(omega);
    const auto alpha = std::sin(omega) / (2.0 * tuning_.highPassQ);
    const auto denominator = 1.0 + alpha;
    const auto numerator = (1.0 + cosine) / (2.0 * denominator);
    highPassCoefficients_ = { numerator, -2.0 * numerator, numerator,
        -2.0 * cosine / denominator, (1.0 - alpha) / denominator };
    highShelfFilter_ = {};
    highShelfFilter_.Prepare(context);
    highShelfFilter_.SetParameters({ 1, static_cast<std::uint32_t>(DspFilterType::HighShelf),
        static_cast<float>(tuning_.highShelfHz), static_cast<float>(tuning_.highShelfDb),
        0.0F, static_cast<float>(tuning_.highShelfQ) }, true);
    Reset();
}

void Cla3aProcessor::Reset() noexcept
{
    input_.Reset();
    output_.Reset();
    detectorPower_ = 0.0;
    detectorShelfStates_ = {};
    fastReductionDb_ = 0.0;
    slowReductionDb_ = 0.0;
    highPassStates_ = {};
    highShelfFilter_.Reset();
}

void Cla3aProcessor::SetControls(double inputGain, double outputGain) noexcept
{
    input_.SetTarget(inputGain);
    output_.SetTarget(outputGain);
}

void Cla3aProcessor::Process(DspAudioBuffer audio, DspChannelMode mode) noexcept
{
    const auto selected = SelectChannels(audio, mode);
    const std::array<double*, 2> channels{ selected.left, selected.right };
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        const auto inputGain = input_.NextValue() * inputCalibration_;
        // The preceding detector state controls gain; this sample charges the optical envelope.
        auto targetDb = 0.0;
        if (detectorPower_ > compressionStartPower_)
        {
            const auto overDb = 10.0 * std::log10(detectorPower_) - tuning_.thresholdDb;
            const auto halfKnee = tuning_.kneeDb * 0.5;
            if (overDb >= halfKnee)
            {
                targetDb = tuning_.compressionSlope * overDb;
            }
            else
            {
                const auto position = overDb + halfKnee;
                targetDb = tuning_.compressionSlope * position * position / (2.0 * tuning_.kneeDb);
            }
        }
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
        const auto detectorAmplitude = amplitude * inputGain;
        const auto power = detectorAmplitude * detectorAmplitude;
        const auto detectorCoefficient = power > detectorPower_
            ? detectorAttackCoefficient_ : detectorReleaseCoefficient_;
        detectorPower_ += detectorCoefficient * (power - detectorPower_);
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
                const auto releaseDrive = std::exp(-targetDb / tuning_.releaseDriveDb);
                if (targetDb < fastReductionDb_)
                {
                    const auto seconds = tuning_.releaseFloorSeconds + tuning_.fastReleaseSeconds * releaseDrive;
                    fastCoefficient = inverseSampleRate_ / (inverseSampleRate_ + seconds);
                }
                if (targetDb < slowReductionDb_)
                {
                    const auto seconds = tuning_.releaseFloorSeconds + tuning_.slowReleaseSeconds * releaseDrive;
                    slowCoefficient = inverseSampleRate_ / (inverseSampleRate_ + seconds);
                }
            }
        }
        fastReductionDb_ += fastCoefficient * (targetDb - fastReductionDb_);
        slowReductionDb_ += slowCoefficient * (targetDb - slowReductionDb_);
        const auto reductionDb = std::lerp(fastReductionDb_, slowReductionDb_, tuning_.slowFraction);
        const auto compressionGain = reductionDb == 0.0 ? 1.0 : DbToGain(-reductionDb);
        const auto& c = highPassCoefficients_;
        for (std::size_t channel = 0; channel < channels.size(); ++channel)
        {
            if (channels[channel] != nullptr)
            {
                auto& state = highPassStates_[channel];
                const auto input = channels[channel][index] * compressionGain;
                const auto output = c[0] * input + state[0];
                state[0] = c[1] * input - c[3] * output + state[1];
                state[1] = c[2] * input - c[4] * output;
                channels[channel][index] = output;
            }
        }
    }
    highShelfFilter_.Process(audio, mode);
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        selected.ApplyGain(index, output_.NextValue() * outputCalibration_);
    }
}

} // namespace consolidator::max::dsp
