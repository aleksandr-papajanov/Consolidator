#include "Dsp/Processors/Compressor/Compressor.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "Dsp/Parameters/ParameterHelper.h"

namespace consolidator::dsp
{

Compressor::Compressor()
{
    RecalculateRuntime();
}

void Compressor::Prepare(double sampleRate, std::size_t channelCount)
{
    assert(sampleRate > 0.0);
    assert(channelCount > 0);

    sampleRate_ = std::max(sampleRate, 1.0);

    sidechain_.Prepare(sampleRate_);

    RecalculateAttackCoefficient();
    RecalculateReleaseCoefficient();

    Reset();
}

void Compressor::Reset() noexcept
{
    sidechain_.Reset();
    rmsDetector_.Reset();

    gainReductionDb_ = 0.0;

    displayedGainReductionDb_.store(0.0f, std::memory_order_relaxed);
}

void Compressor::Process(
    const double* input,
    double* output,
    std::size_t frameCount,
    std::size_t channelCount)
{
    assert(input != nullptr);
    assert(output != nullptr);

    const auto sampleCount = frameCount * channelCount;

    if (state_.bypass)
    {
        std::copy_n(input, sampleCount, output);
        displayedGainReductionDb_.store(0.0f, std::memory_order_relaxed);
        return;
    }

    double lastGainReductionDb = gainReductionDb_;

    for (std::size_t frame = 0; frame < frameCount; ++frame)
    {
        const auto frameOffset = frame * channelCount;

        const double detectorInput = CalculateLinkedDetectorInput(input + frameOffset, channelCount);

        const double inputLevelDb = MeasureLevelDb(detectorInput);
        const double targetGainReductionDb = CalculateTargetGainReductionDb(inputLevelDb);
        const double smoothedGainReductionDb = UpdateGainReductionDb(targetGainReductionDb);

        const double gainLinear = std::pow(10.0, smoothedGainReductionDb / 20.0);

        for (std::size_t channel = 0; channel < channelCount; ++channel)
        {
            const auto sampleIndex = frameOffset + channel;

            output[sampleIndex] = ProcessSample(input[sampleIndex], gainLinear);
        }

        lastGainReductionDb = smoothedGainReductionDb;
    }

    displayedGainReductionDb_.store(
        static_cast<float>(lastGainReductionDb),
        std::memory_order_relaxed);
}

double Compressor::CalculateLinkedDetectorInput(const double* frame, std::size_t channelCount) noexcept
{
    double linkedPeak = 0.0;

    for (std::size_t channel = 0; channel < channelCount; ++channel)
    {
        linkedPeak = std::max(linkedPeak, std::abs(frame[channel]));
    }

    return sidechain_.ProcessSample(linkedPeak);
}

double Compressor::MeasureLevelDb(double detectorInput) noexcept
{
    const double rmsLevel = rmsDetector_.ProcessSample(detectorInput);
    const double safeLevel = std::max(rmsLevel, kMinimumLevelLinear);

    return 20.0 * std::log10(safeLevel);
}

double Compressor::CalculateTargetGainReductionDb(double inputLevelDb) const noexcept
{
    const double thresholdDb = static_cast<double>(state_.thresholdDb);
    const double ratio = static_cast<double>(state_.ratio);
    const double levelAboveThreshold = inputLevelDb - thresholdDb;
    const double halfKnee = kSoftKneeWidthDb * 0.5;

    if (levelAboveThreshold <= -halfKnee)
    {
        return 0.0;
    }

    const double compressionSlope = 1.0 - (1.0 / ratio);

    double gainReductionDb = 0.0;

    if (levelAboveThreshold >= halfKnee)
    {
        gainReductionDb = -levelAboveThreshold * compressionSlope;
    }
    else
    {
        const double kneePosition = levelAboveThreshold + halfKnee;

        gainReductionDb =
            -(compressionSlope * kneePosition * kneePosition) /
            (2.0 * kSoftKneeWidthDb);
    }

    return std::max(gainReductionDb, kMinimumGainReductionDb);
}

double Compressor::UpdateGainReductionDb(double targetGainReductionDb) noexcept
{
    const bool isIncreasingCompression = targetGainReductionDb < gainReductionDb_;

    const double coefficient = isIncreasingCompression
        ? runtime_.attackCoefficient
        : runtime_.releaseCoefficient;

    gainReductionDb_ =
        coefficient * gainReductionDb_ +
        (1.0 - coefficient) * targetGainReductionDb;

    return gainReductionDb_;
}

double Compressor::ProcessSample(double input, double gainLinear) const noexcept
{
    const double compressed = input * gainLinear * runtime_.outputGainLinear;

    return compressed * runtime_.wetMix + input * runtime_.dryMix;
}

void Compressor::ApplyParameterChange(const ParameterChange& change)
{
    if (IsDetectorParameter(change))
    {
        ApplyDetectorParameter(change);
        return;
    }

    ApplyCompressorParameter(change);
}

bool Compressor::IsDetectorParameter(const ParameterChange& change) const noexcept
{
    return change.address.GetElementKind() == detail::ElementKind::CompressorDetectorFilter;
}

void Compressor::ApplyDetectorParameter(const ParameterChange& change)
{
    sidechain_.ApplyParameterChange(change);
}

void Compressor::ApplyCompressorParameter(const ParameterChange& change)
{
    switch (change.address.GetParameterId())
    {
    case ParameterId::Threshold:
        if (const auto* value = TryGetValue<float>(change))
        {
            SetThreshold(*value);
        }
        break;

    case ParameterId::Ratio:
        if (const auto* value = TryGetValue<float>(change))
        {
            SetRatio(*value);
        }
        break;

    case ParameterId::Attack:
        if (const auto* value = TryGetValue<float>(change))
        {
            SetAttack(*value);
        }
        break;

    case ParameterId::Release:
        if (const auto* value = TryGetValue<float>(change))
        {
            SetRelease(*value);
        }
        break;

    case ParameterId::Gain:
        if (const auto* value = TryGetValue<float>(change))
        {
            SetOutputDb(*value);
        }
        break;

    case ParameterId::Mix:
        if (const auto* value = TryGetValue<float>(change))
        {
            SetMix(*value);
        }
        break;

    case ParameterId::Bypass:
        if (const auto* value = TryGetValue<bool>(change))
        {
            SetBypass(*value);
        }
        break;

    default:
        break;
    }
}

void Compressor::SetThreshold(float thresholdDb) noexcept
{
    state_.thresholdDb = std::clamp(
        thresholdDb,
        kMinimumThresholdDb,
        kMaximumThresholdDb);
}

void Compressor::SetRatio(float ratio) noexcept
{
    state_.ratio = std::clamp(ratio, kMinimumRatio, kMaximumRatio);
}

void Compressor::SetAttack(float attackMs) noexcept
{
    state_.attackMs = std::clamp(
        attackMs,
        kMinimumAttackMs,
        kMaximumAttackMs);

    RecalculateAttackCoefficient();
}

void Compressor::SetRelease(float releaseMs) noexcept
{
    state_.releaseMs = std::clamp(
        releaseMs,
        kMinimumReleaseMs,
        kMaximumReleaseMs);

    RecalculateReleaseCoefficient();
}

void Compressor::SetOutputDb(float outputDb)
{
    state_.outputDb = std::clamp(
        outputDb,
        kMinimumOutputDb,
        kMaximumOutputDb);

    RecalculateOutputGain();
}

void Compressor::SetMix(float mix) noexcept
{
    state_.mix = std::clamp(mix, kMinimumMix, kMaximumMix);
    RecalculateMix();
}

void Compressor::SetBypass(bool bypass) noexcept
{
    state_.bypass = bypass;
}

void Compressor::RecalculateRuntime()
{
    RecalculateAttackCoefficient();
    RecalculateReleaseCoefficient();
    RecalculateOutputGain();
    RecalculateMix();
}

void Compressor::RecalculateAttackCoefficient() noexcept
{
    runtime_.attackCoefficient = CalculateTimeCoefficient(state_.attackMs, sampleRate_);
}

void Compressor::RecalculateReleaseCoefficient() noexcept
{
    runtime_.releaseCoefficient = CalculateTimeCoefficient(state_.releaseMs, sampleRate_);
}

void Compressor::RecalculateOutputGain()
{
    runtime_.outputGainLinear = std::pow(10.0, static_cast<double>(state_.outputDb) / 20.0);
}

void Compressor::RecalculateMix() noexcept
{
    runtime_.wetMix = static_cast<double>(state_.mix);
    runtime_.dryMix = 1.0 - runtime_.wetMix;
}

double Compressor::CalculateTimeCoefficient(double timeMs, double sampleRate) noexcept
{
    const double safeTimeMs = std::max(timeMs, 0.01);
    const double safeSampleRate = std::max(sampleRate, 1.0);
    const double timeSeconds = safeTimeMs * 0.001;

    return std::exp(-1.0 / (timeSeconds * safeSampleRate));
}

} // namespace consolidator::dsp