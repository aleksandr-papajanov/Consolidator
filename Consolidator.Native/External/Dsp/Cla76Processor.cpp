#include "Cla76Processor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "DspMath.h"
#include "DspProcessingPrimitives.h"

namespace consolidator::max::dsp
{

namespace
{

double AttackLogTime(double position, double bend, const std::array<double, 3>& coefficients) noexcept
{
    return coefficients[0] + position * coefficients[1] + bend * coefficients[2];
}

double PositiveControlSurface(double attack, double release, const std::array<double, 6>& coefficients) noexcept
{
    return std::exp(coefficients[0] + attack * (coefficients[1] + attack * coefficients[3]) +
        release * (coefficients[2] + release * coefficients[5]) + attack * release * coefficients[4]);
}

template <std::size_t Count>
double ControlPolynomial(double control, const std::array<double, Count>& coefficients) noexcept
{
    auto value = coefficients.back();
    for (std::size_t index = Count - 1; index > 0; --index)
    {
        value = value * control + coefficients[index - 1];
    }
    return value;
}

struct AllControlValues
{
    double thresholdOffsetDb;
    double slopeOffset;
    double memoryLimitDb;
    double memorySeconds;
};

AllControlValues EvaluateAllControls(double attack, double release, const Cla76Tuning& tuning) noexcept
{
    return {
        ControlPolynomial(release, tuning.allThresholdLaw),
        ControlPolynomial(release, tuning.allSlopeLaw),
        PositiveControlSurface(attack, release, tuning.allMemoryLimitLaw),
        PositiveControlSurface(attack, release, tuning.allMemoryTimeLaw)
    };
}

} // namespace

void Cla76Processor::Prepare(const DspProcessingContext& context) noexcept
{
    sampleRate_ = context.sampleRate;
    inputCalibration_ = DbToGain(tuning_.inputCalibrationDb);
    outputCalibration_ = DbToGain(tuning_.outputCalibrationDb);
    inputGain_.Prepare(context);
    outputGain_.Prepare(context);
    attack_.Prepare(context);
    release_.Prepare(context);
    ratio_.Prepare(context);
    const auto tangent = std::tan(3.141592653589793 *
        std::min(tuning_.colourPoleHz, sampleRate_ * 0.499) / sampleRate_);
    // Keep the unity-DC filter attenuating at lower sample rates as well.
    colourPole_ = std::max((1.0 - tangent) / (1.0 + tangent), 1.0 - 2.0 * tuning_.colourFeedthrough);
    colourNumerator_ = 1.0 - colourPole_ - tuning_.colourFeedthrough;
    Reset();
}

void Cla76Processor::Reset() noexcept
{
    inputGain_.Reset();
    outputGain_.Reset();
    attack_.Reset();
    release_.Reset();
    ratio_.Reset();
    detectorPower_ = 0.0;
    allMemoryReductionDb_ = 0.0;
    colourPoleStates_ = {};
    cachedAttack_ = -1.0;
    cachedRelease_ = -1.0;
    cachedRatio_ = -1.0;
}

void Cla76Processor::SetControls(
    double inputGain, double outputGain, double attack, double release, double ratio) noexcept
{
    inputGain_.SetTarget(inputGain);
    outputGain_.SetTarget(outputGain);
    // Public controls increase with time; fitted laws use the reference knob orientation.
    attack_.SetTarget(7.0 - 6.0 * attack);
    release_.SetTarget(7.0 - 6.0 * release);
    ratio_.SetTarget(ratio);
}

void Cla76Processor::Process(DspAudioBuffer audio, DspChannelMode mode) noexcept
{
    const auto selected = SelectChannels(audio, mode);
    const std::array<double*, 2> channels{ selected.left, selected.right };
    for (std::size_t index = 0; index < audio.frameCount; ++index)
    {
        const auto inputGain = inputGain_.NextValue() * inputCalibration_;
        const auto attack = attack_.NextValue();
        const auto release = release_.NextValue();
        const auto ratio = ratio_.NextValue();
        if (ratio != cachedRatio_)
        {
            const auto& normal = tuning_.normal;
            const auto& all = tuning_.all;
            activeMode_.thresholdDb = std::lerp(normal.thresholdDb, all.thresholdDb, ratio);
            activeMode_.thresholdAttackSpanDb = std::lerp(
                normal.thresholdAttackSpanDb, all.thresholdAttackSpanDb, ratio);
            activeMode_.kneeDb = std::lerp(normal.kneeDb, all.kneeDb, ratio);
            activeMode_.compressionSlope = std::lerp(normal.compressionSlope, all.compressionSlope, ratio);
            activeMode_.outputOffsetDb = std::lerp(normal.outputOffsetDb, all.outputOffsetDb, ratio);
            modeOutputGain_ = DbToGain(activeMode_.outputOffsetDb);
            for (std::size_t coefficient = 0; coefficient < activeMode_.attackLaw.size(); ++coefficient)
            {
                activeMode_.attackLaw[coefficient] = std::lerp(
                    normal.attackLaw[coefficient], all.attackLaw[coefficient], ratio);
            }
            for (std::size_t coefficient = 0; coefficient < activeMode_.releaseLaw.size(); ++coefficient)
            {
                activeMode_.releaseLaw[coefficient] = std::lerp(
                    normal.releaseLaw[coefficient], all.releaseLaw[coefficient], ratio);
            }
            cachedRatio_ = ratio;
            cachedAttack_ = -1.0;
            cachedRelease_ = -1.0;
        }
        const auto timingChanged = attack != cachedAttack_ || release != cachedRelease_;
        if (timingChanged)
        {
            const auto position = (attack - 4.0) / 3.0;
            const auto bend = std::abs(position);
            const auto baseLog = AttackLogTime(position, bend, activeMode_.attackLaw);
            const auto allLog = AttackLogTime(position, bend, tuning_.all.attackLaw);
            const auto carrierLog = AttackLogTime(position, bend, tuning_.allCarrierAttackLaw);
            const auto amount = ratio * std::clamp((release - 4.0) / 3.0, 0.0, 1.0);
            const auto seconds = std::exp(baseLog + amount * (carrierLog - allLog));
            attackCoefficient_ = TimeCoefficient(seconds, sampleRate_);
            cachedAttack_ = attack;
        }
        if (release != cachedRelease_)
        {
            const auto seconds = std::exp(ControlPolynomial((release - 4.0) / 3.0, activeMode_.releaseLaw));
            releaseCoefficient_ = TimeCoefficient(seconds, sampleRate_);
            cachedRelease_ = release;
        }
        if (timingChanged)
        {
            const auto attackPosition = (attack - 4.0) / 3.0;
            const auto baseThresholdDb = activeMode_.thresholdDb + activeMode_.thresholdAttackSpanDb * attackPosition;
            const auto releasePosition = (release - 4.0) / 3.0;
            const auto controls = EvaluateAllControls(attackPosition, releasePosition, tuning_);
            const auto fastCurveAmount = ratio * attackPosition * std::clamp((release - 5.0) / 2.0, 0.0, 1.0);
            activeThresholdDb_ = baseThresholdDb + ratio * controls.thresholdOffsetDb +
                fastCurveAmount * tuning_.allFastReleaseThresholdAttackSpanDb;
            activeCompressionSlope_ = activeMode_.compressionSlope + ratio * controls.slopeOffset +
                fastCurveAmount * tuning_.allFastReleaseSlopeAttackSpan;
            allMemoryLimitDb_ = controls.memoryLimitDb;
            allMemoryCoefficient_ = TimeCoefficient(
                controls.memorySeconds, sampleRate_);
            allReleaseCurrent_ = tuning_.allReleaseCurrentRatio * std::sqrt(DbToGain(baseThresholdDb));
            const auto compressionStart = DbToGain(activeThresholdDb_ - activeMode_.kneeDb * 0.5);
            compressionStartPower_ = compressionStart * compressionStart;
        }
        // Apply the envelope established by preceding samples, before charging it with this sample.
        auto targetDb = 0.0;
        if (detectorPower_ > compressionStartPower_)
        {
            const auto overDb = 10.0 * std::log10(detectorPower_) - activeThresholdDb_;
            const auto halfKnee = activeMode_.kneeDb * 0.5;
            if (overDb >= halfKnee)
            {
                targetDb = activeCompressionSlope_ * overDb;
            }
            else
            {
                const auto position = overDb + halfKnee;
                targetDb = activeCompressionSlope_ * position * position / (2.0 * activeMode_.kneeDb);
            }
        }
        const auto detectorAmplitude = selected.Peak(index) * inputGain;
        const auto detectorInput = detectorAmplitude * detectorAmplitude;
        if (detectorInput > detectorPower_)
        {
            detectorPower_ += attackCoefficient_ * (detectorInput - detectorPower_);
        }
        else
        {
            // Release in amplitude keeps the recovery tail broader than power-domain decay.
            const auto amplitude = std::sqrt(detectorPower_);
            auto released = amplitude + releaseCoefficient_ * (detectorAmplitude - amplitude);
            if (ratio > 0.0)
            {
                const auto voltage = std::max(0.0, std::sqrt(amplitude) - releaseCoefficient_ * allReleaseCurrent_);
                released = std::lerp(released, voltage * voltage, ratio);
            }
            detectorPower_ = released * released;
        }
        // ALL separates the bounded, slowly settling reduction from the fast peak response.
        // The same history stays warm while Ratio moves; at 4:1 its contribution is exactly zero.
        const auto memoryTargetDb = std::min(targetDb, allMemoryLimitDb_);
        allMemoryReductionDb_ += allMemoryCoefficient_ * (memoryTargetDb - allMemoryReductionDb_);
        const auto reductionDb = targetDb + ratio *
            (std::min(allMemoryReductionDb_, memoryTargetDb) - memoryTargetDb);
        const auto compressionGain = reductionDb == 0.0 ? 1.0 : DbToGain(-reductionDb);
        const auto processedGain = inputGain * compressionGain * modeOutputGain_;
        const auto outputGain = outputGain_.NextValue() * outputCalibration_;
        for (std::size_t channel = 0; channel < channels.size(); ++channel)
        {
            if (channels[channel] != nullptr)
            {
                const auto input = channels[channel][index] * processedGain;
                const auto output = tuning_.colourFeedthrough * input + colourPoleStates_[channel];
                colourPoleStates_[channel] = colourNumerator_ * input + colourPole_ * output;
                channels[channel][index] = output * outputGain;
            }
        }
    }
}

} // namespace consolidator::max::dsp
