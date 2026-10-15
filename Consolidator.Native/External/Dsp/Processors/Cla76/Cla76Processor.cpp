#include "Cla76Processor.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>

#include "../../DspMath.h"
#include "../../DspProcessingPrimitives.h"
#include "../CompressorMakeup.h"

namespace consolidator::max::dsp::processors
{

namespace
{

// Recovered analytic control tapers; full precision avoids cancellation in the rational functions.
constexpr std::array<double, 9> kInputGainNumerator{
    1.2415210877230192, -41.86464583815966, 601.7934701282558,
    -4585.030060488159, 19600.824591939432, -47225.554659362,
    63349.09064748866, -44975.004283564645, 14076.427526040545 };

constexpr std::array<double, 8> kInputGainDenominator{
    -35.998016694314074, 562.7658609940733, -4685.9341403056305,
    22099.995578662485, -60139.628671453844, 94528.92187601369,
    -81559.13215595554, 30879.678463057153 };

constexpr std::array<double, 9> kOutputGainNumerator{
    0.15997234780526612, -4.68207659299861, 61.34421205000684,
    -463.82613770766187, 2132.8710786598367, -5821.110759896061,
    9174.329509886406, -7829.794796488582, 2909.7334456769845 };

constexpr std::array<double, 8> kOutputGainDenominator{
    -27.67807914481682, 327.63454423694225, -2165.9068264273283,
    8729.398442254662, -21735.19625554911, 32531.7338988588,
    -26901.87192064749, 9541.452992358421 };

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

double AttenuatorGain(double nominalGain, const std::array<double, 9>& numerator,
    const std::array<double, 8>& denominator) noexcept
{
    const auto position = std::sqrt(nominalGain);
    return nominalGain * ControlPolynomial(position, numerator) /
        (1.0 + position * ControlPolynomial(position, denominator));
}

struct AllControlValues
{
    double thresholdOffsetDb;
    double slopeOffset;
    double memoryLimitDb;
    double memorySeconds;
};

AllControlValues EvaluateAllControls(double attack, double release, const Cla76Config& config) noexcept
{
    return {
        ControlPolynomial(release, config.allThresholdLaw),
        ControlPolynomial(release, config.allSlopeLaw),
        PositiveControlSurface(attack, release, config.allMemoryLimitLaw),
        PositiveControlSurface(attack, release, config.allMemoryTimeLaw)
    };
}

} // namespace

Cla76Processor::Cla76Processor(CompressorControlMode controlMode) noexcept
    : controlMode_(controlMode),
      inputGain_(controlMode == CompressorControlMode::Emulation
          ? AttenuatorGain(Cla76EmulationControls{}.inputGain, kInputGainNumerator, kInputGainDenominator) : 1.0),
      outputGain_(controlMode == CompressorControlMode::Emulation
          ? AttenuatorGain(Cla76EmulationControls{}.outputGain, kOutputGainNumerator, kOutputGainDenominator) : 1.0),
      ratio_(controlMode == CompressorControlMode::Emulation
          ? Cla76EmulationControls{}.ratio : NormalRatioPosition(Cla76Controls{}.ratio))
{
}

void Cla76Processor::Prepare(const DspProcessingContext& context) noexcept
{
    sampleRate_ = context.sampleRate;
    inputCalibration_ = controlMode_ == CompressorControlMode::Emulation
        ? DbToGain(kConfig.inputCalibrationDb) : 1.0;
    outputCalibration_ = controlMode_ == CompressorControlMode::Emulation
        ? DbToGain(kConfig.outputCalibrationDb) : 1.0;
    reduction_.Prepare(context);
    inputGain_.Prepare(context);
    outputGain_.Prepare(context);
    attack_.Prepare(context);
    release_.Prepare(context);
    ratio_.Prepare(context);
    const auto tangent = std::tan(3.141592653589793 *
        std::min(kConfig.colourPoleHz, sampleRate_ * 0.499) / sampleRate_);
    // Keep the unity-DC filter attenuating at lower sample rates as well.
    colourPole_ = std::max((1.0 - tangent) / (1.0 + tangent), 1.0 - 2.0 * kConfig.colourFeedthrough);
    colourNumerator_ = 1.0 - colourPole_ - kConfig.colourFeedthrough;
    Reset();
}

void Cla76Processor::Reset() noexcept
{
    reduction_.Reset();
    cachedReduction_ = -1.0;
    inputGain_.Reset();
    outputGain_.Reset();
    attack_.Reset();
    release_.Reset();
    ratio_.Reset();
    detectorPower_ = 0.0;
    allMemoryReductionDb_ = 0.0;
    colourPoleStates_ = {};
    cachedNominalInput_ = -1.0;
    cachedNominalOutput_ = -1.0;
    cachedAttack_ = -1.0;
    cachedRelease_ = -1.0;
    cachedRatio_ = -1.0;
    cachedRatioControl_ = -1.0;
}

void Cla76Processor::SetControls(const Cla76Controls& controls) noexcept
{
    if (controlMode_ == CompressorControlMode::Emulation)
    {
        const auto& reference = controls.emulation;
        if (reference.inputGain != cachedNominalInput_)
        {
            inputGain_.SetTarget(AttenuatorGain(reference.inputGain, kInputGainNumerator, kInputGainDenominator));
            cachedNominalInput_ = reference.inputGain;
        }
        if (reference.outputGain != cachedNominalOutput_)
        {
            outputGain_.SetTarget(AttenuatorGain(reference.outputGain, kOutputGainNumerator, kOutputGainDenominator));
            cachedNominalOutput_ = reference.outputGain;
        }
        attack_.SetTarget(reference.attack);
        release_.SetTarget(reference.release);
        ratio_.SetTarget(reference.ratio);
    }
    else
    {
        reduction_.SetTarget(controls.reduction);
        outputGain_.SetTarget(controls.outputGain);
        attack_.SetTarget(7.0 - 6.0 * controls.attack);
        release_.SetTarget(7.0 - 6.0 * controls.release);
        ratio_.SetTarget(NormalRatioPosition(controls.ratio));
    }
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
        const auto ratioControl = ratio_.NextValue();
        // Preserve the old 4:1-to-ALL coordinate above 4:1; the lower section has no ALL contribution.
        const auto ratio = controlMode_ == CompressorControlMode::Emulation
            ? ratioControl : std::max((99.0 * ratioControl - 3.0) / 96.0, 0.0);
        UpdateMode(ratio);
        const auto reduction = reduction_.NextValue();
        UpdateControlCoefficients(attack, release, ratio, reduction, ratioControl);
        // Apply the envelope established by preceding samples, before charging it with this sample.
        const auto targetDb = controlMode_ == CompressorControlMode::Normal && reduction == 0.0
            ? 0.0 : CalculateTargetReductionDb();
        const auto detectorAmplitude = selected.Peak(index) * inputGain;
        UpdateDetector(detectorAmplitude, ratio);
        const auto reductionDb = UpdateMemoryReductionDb(targetDb, ratio);
        const auto compressionGain = reductionDb == 0.0 ? 1.0 : DbToGain(-reductionDb);
        const auto processedGain = controlMode_ == CompressorControlMode::Emulation
            ? inputGain * compressionGain * modeOutputGain_ : compressionGain * makeupGain_;
        const auto outputGain = outputGain_.NextValue() * outputCalibration_;
        for (std::size_t channel = 0; channel < channels.size(); ++channel)
        {
            if (channels[channel] != nullptr)
            {
                channels[channel][index] = ProcessColourSample(
                    channels[channel][index] * processedGain, channel) * outputGain;
            }
        }
    }
}

void Cla76Processor::UpdateMode(double ratio) noexcept
{
    if (ratio != cachedRatio_)
    {
        const auto& normal = kConfig.normal;
        const auto& all = kConfig.all;
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
        // Mode morphing changes the timing laws even when the timing knobs stay still.
        cachedAttack_ = -1.0;
        cachedRelease_ = -1.0;
    }
}

void Cla76Processor::UpdateControlCoefficients(
    double attack, double release, double ratio, double reduction, double ratioControl) noexcept
{
    const auto timingChanged = attack != cachedAttack_ || release != cachedRelease_;
    if (timingChanged)
    {
        const auto position = (attack - 4.0) / 3.0;
        const auto bend = std::abs(position);
        const auto baseLog = AttackLogTime(position, bend, activeMode_.attackLaw);
        const auto allLog = AttackLogTime(position, bend, kConfig.all.attackLaw);
        const auto carrierLog = AttackLogTime(position, bend, kConfig.allCarrierAttackLaw);
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
    if (timingChanged || reduction != cachedReduction_ || ratioControl != cachedRatioControl_)
    {
        const auto attackPosition = (attack - 4.0) / 3.0;
        const auto threshold = controlMode_ == CompressorControlMode::Emulation
            ? activeMode_.thresholdDb
            : kNormalThresholdMaximumDb - kNormalThresholdSpanDb * reduction +
                activeMode_.thresholdDb - kConfig.normal.thresholdDb;
        const auto baseThresholdDb = threshold + activeMode_.thresholdAttackSpanDb * attackPosition;
        const auto releasePosition = (release - 4.0) / 3.0;
        const auto controls = EvaluateAllControls(attackPosition, releasePosition, kConfig);
        const auto fastCurveAmount = ratio * attackPosition * std::clamp((release - 5.0) / 2.0, 0.0, 1.0);
        activeThresholdDb_ = baseThresholdDb + ratio * controls.thresholdOffsetDb +
            fastCurveAmount * kConfig.allFastReleaseThresholdAttackSpanDb;
        activeCompressionSlope_ = activeMode_.compressionSlope + ratio * controls.slopeOffset +
            fastCurveAmount * kConfig.allFastReleaseSlopeAttackSpan;
        if (controlMode_ == CompressorControlMode::Normal && ratioControl < kNormalRatioFourToOnePosition)
        {
            activeCompressionSlope_ = 1.0 - 1.0 / (1.0 + 99.0 * ratioControl);
        }
        allMemoryLimitDb_ = controls.memoryLimitDb;
        allMemoryCoefficient_ = TimeCoefficient(
            controls.memorySeconds, sampleRate_);
        allReleaseCurrent_ = kConfig.allReleaseCurrentRatio * std::sqrt(DbToGain(baseThresholdDb));
        const auto compressionStart = DbToGain(activeThresholdDb_ - activeMode_.kneeDb * 0.5);
        compressionStartPower_ = compressionStart * compressionStart;
        if (controlMode_ == CompressorControlMode::Normal)
        {
            makeupGain_ = Cla76ProgramMakeupGain(
                reduction, (7.0 - attack) / 6.0, (7.0 - release) / 6.0, ratioControl);
        }
        cachedReduction_ = reduction;
        cachedRatioControl_ = ratioControl;
    }
}

double Cla76Processor::CalculateTargetReductionDb() const noexcept
{
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
    return targetDb;
}

void Cla76Processor::UpdateDetector(double detectorAmplitude, double ratio) noexcept
{
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
}

double Cla76Processor::UpdateMemoryReductionDb(double targetDb, double ratio) noexcept
{
    // ALL separates the bounded, slowly settling reduction from the fast peak response.
    // The same history stays warm while Ratio moves; at 4:1 its contribution is exactly zero.
    const auto memoryTargetDb = std::min(targetDb, allMemoryLimitDb_);
    allMemoryReductionDb_ += allMemoryCoefficient_ * (memoryTargetDb - allMemoryReductionDb_);
    const auto reductionDb = targetDb + ratio *
        (std::min(allMemoryReductionDb_, memoryTargetDb) - memoryTargetDb);
    return reductionDb;
}

double Cla76Processor::ProcessColourSample(double input, std::size_t channel) noexcept
{
    const auto output = kConfig.colourFeedthrough * input + colourPoleStates_[channel];
    colourPoleStates_[channel] = colourNumerator_ * input + colourPole_ * output;
    return output;
}

} // namespace consolidator::max::dsp::processors
