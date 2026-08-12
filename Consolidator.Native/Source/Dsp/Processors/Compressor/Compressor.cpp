#include "Dsp/Processors/Compressor/Compressor.h"

#include <algorithm>
#include <cassert>
#include <cmath>
#include <memory>

#include "Dsp/Utilities/TimeCoefficient.h"
#include "Dsp/Processors/Equalizer/Filters/BellFilter.h"
#include "Dsp/Processors/Equalizer/Filters/LowShelfFilter.h"

namespace consolidator::dsp
{

Compressor::Compressor()
    : DspDevice(DeviceId::Compressor, detail::ElementKind::Device, 0)
{
    detectorEqualizer_.AddFilter(std::make_unique<LowShelfFilter>(
        FilterId::Filter1,
        core::settings::DetectorDefaults::kDefaultLowShelfFrequencyHz));

    detectorEqualizer_.AddFilter(std::make_unique<BellFilter>(
        FilterId::Filter2,
        core::settings::DetectorDefaults::kDefaultBellFrequencyHz));

    RecalculateRuntime();
}

void Compressor::Prepare(double sampleRate, std::size_t channelCount)
{
    assert(sampleRate > 0.0);
    assert(channelCount > 0);

    runtimeState_.sampleRate = std::max(sampleRate, 1.0);

    detectorEqualizer_.Prepare(runtimeState_.sampleRate, 1);

    RecalculateAttackCoefficient();
    RecalculateReleaseCoefficient();

    Reset();
}

void Compressor::Reset() noexcept
{
    detectorEqualizer_.Reset();
    rmsDetector_.Reset();

    runtimeState_.gainReductionDb = 0.0;
    detectorMonitoringSample_ = 0.0;

    meterState_.gainReductionDb.store(0.0f, std::memory_order_relaxed);
    meterState_.attenuationSumSquares = 0.0;
    meterState_.minimumAttenuation = 1.0f;
    meterState_.attenuationSampleCount = 0;
}

bool Compressor::Reset(
    const core::StatePath& route,
    std::size_t depth) noexcept
{
    if (route.GetDeviceId() != GetDeviceId())
    {
        return false;
    }

    if (depth == route.GetDepth())
    {
        return DspDevice::Reset(route, depth);
    }

    if (route.GetNode(depth) != RouteNodeId::Detector)
    {
        return false;
    }

    return detectorEqualizer_.Reset(route, depth + 1);
}

void Compressor::Process(
    const double* inputLeft,
    const double* inputRight,
    double* outputLeft,
    double* outputRight,
    std::size_t frameCount)
{
    assert(inputLeft != nullptr);
    assert(inputRight != nullptr);
    assert(outputLeft != nullptr);
    assert(outputRight != nullptr);

    double lastGainReductionDb = runtimeState_.gainReductionDb;

    for (std::size_t frame = 0; frame < frameCount; ++frame)
    {
        const double detectorInput = CalculateLinkedDetectorInput(
            std::max(std::abs(inputLeft[frame]), std::abs(inputRight[frame])));

        const double inputLevelDb = MeasureLevelDb(detectorInput);
        const double targetGainReductionDb = CalculateTargetGainReductionDb(inputLevelDb);
        const double smoothedGainReductionDb = UpdateGainReductionDb(targetGainReductionDb);

        const double gainLinear = std::pow(10.0, smoothedGainReductionDb / 20.0);

        if (detectorListen_)
        {
            outputLeft[frame] = detectorMonitoringSample_;
            outputRight[frame] = detectorMonitoringSample_;
        }
        else
        {
            outputLeft[frame] = ProcessSample(inputLeft[frame], gainLinear);
            outputRight[frame] = ProcessSample(inputRight[frame], gainLinear);
        }

        lastGainReductionDb = smoothedGainReductionDb;
        if (meterState_.telemetryEnabled)
        {
            const auto attenuation = static_cast<float>(std::pow(
                10.0, smoothedGainReductionDb / 20.0));
            meterState_.attenuationSumSquares += attenuation * attenuation;
            meterState_.minimumAttenuation = std::min(
                meterState_.minimumAttenuation,
                attenuation);
            ++meterState_.attenuationSampleCount;
        }
    }

    meterState_.gainReductionDb.store(
        static_cast<float>(lastGainReductionDb),
        std::memory_order_relaxed);
}

double Compressor::CalculateLinkedDetectorInput(double linkedInput) noexcept
{
    detectorMonitoringSample_ = detectorEqualizer_.ProcessSample(linkedInput);
    return detectorMonitoringSample_;
}

double Compressor::MeasureLevelDb(double detectorInput) noexcept
{
    const double rmsLevel = rmsDetector_.ProcessSample(detectorInput);
    const double safeLevel = std::max(
        rmsLevel,
        core::settings::CompressorDefaults::kMinimumLevelLinear);

    return 20.0 * std::log10(safeLevel);
}

double Compressor::CalculateTargetGainReductionDb(double inputLevelDb) const noexcept
{
    const double thresholdDb = static_cast<double>(runtimeState_.thresholdDb);
    const double ratio = static_cast<double>(runtimeState_.ratio);
    const double levelAboveThreshold = inputLevelDb - thresholdDb;
    const double halfKnee = core::settings::CompressorDefaults::kSoftKneeWidthDb * 0.5;

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
            (2.0 * core::settings::CompressorDefaults::kSoftKneeWidthDb);
    }

    return std::max(
        gainReductionDb,
        core::settings::CompressorDefaults::kMinimumGainReductionDb);
}

double Compressor::UpdateGainReductionDb(double targetGainReductionDb) noexcept
{
    const bool isIncreasingCompression = targetGainReductionDb < runtimeState_.gainReductionDb;

    const double coefficient = isIncreasingCompression
        ? runtimeState_.attackCoefficient
        : runtimeState_.releaseCoefficient;

    runtimeState_.gainReductionDb =
        coefficient * runtimeState_.gainReductionDb +
        (1.0 - coefficient) * targetGainReductionDb;

    return runtimeState_.gainReductionDb;
}

double Compressor::ProcessSample(double input, double gainLinear) const noexcept
{
    const double compressed = input * gainLinear * runtimeState_.outputGainLinear;

    return compressed * runtimeState_.wetMix + input * runtimeState_.dryMix;
}

bool Compressor::ApplyOwnParameter(
    const core::StatePath& route,
    const ParameterVariant& value)
{
    const auto parameterId = route.GetParameterId();
    if (parameterId == ParameterId::Threshold) { const auto* v = std::get_if<float>(&value); if (v == nullptr) return false; runtimeState_.thresholdDb = *v; return true; }
    if (parameterId == ParameterId::Ratio) { const auto* v = std::get_if<float>(&value); if (v == nullptr) return false; runtimeState_.ratio = *v; return true; }
    if (parameterId == ParameterId::Attack) { const auto* v = std::get_if<float>(&value); if (v == nullptr) return false; runtimeState_.attackMs = *v; return true; }
    if (parameterId == ParameterId::Release) { const auto* v = std::get_if<float>(&value); if (v == nullptr) return false; runtimeState_.releaseMs = *v; return true; }
    if (parameterId == ParameterId::Gain) { const auto* v = std::get_if<float>(&value); if (v == nullptr) return false; runtimeState_.outputDb = *v; return true; }
    if (parameterId == ParameterId::Mix) { const auto* v = std::get_if<float>(&value); if (v == nullptr) return false; runtimeState_.mix = *v; return true; }
    return false;
}

CompressorBlockTelemetry Compressor::GetBlockTelemetry() const noexcept
{
    if (meterState_.attenuationSampleCount == 0)
    {
        return {};
    }

    return {
        static_cast<float>(-20.0 * std::log10(std::max(
            std::sqrt(
                meterState_.attenuationSumSquares /
                static_cast<double>(meterState_.attenuationSampleCount)),
            1.0e-12))),
        static_cast<float>(-20.0 * std::log10(std::max(
            static_cast<double>(meterState_.minimumAttenuation),
            1.0e-12)))};
}

void Compressor::ResetBlockTelemetry() noexcept
{
    meterState_.attenuationSumSquares = 0.0;
    meterState_.minimumAttenuation = 1.0f;
    meterState_.attenuationSampleCount = 0;
}

bool Compressor::ApplyParameter(
    const core::StatePath& route,
    const ParameterVariant& value,
    std::size_t depth)
{
    if (route.GetDeviceId() != GetDeviceId())
    {
        return false;
    }

    if (depth == route.GetDepth())
    {
        return DspDevice::ApplyParameter(route, value, depth);
    }

    if (route.GetNode(depth) != RouteNodeId::Detector)
    {
        return false;
    }

    auto equalizerRoute = route;
    equalizerRoute.deviceId = DeviceId::Equalizer;
    const bool isUpdated = detectorEqualizer_.ApplyParameter(
        equalizerRoute,
        value,
        depth + 1);
    return isUpdated;
}

bool Compressor::ApplyProcessingStateAtDepth(
    const core::StatePath& target,
    bool active,
    std::size_t depth)
{
    if (target.GetDeviceId() != GetDeviceId())
    {
        return false;
    }
    if (depth == target.GetDepth())
    {
        return DspDevice::ApplyProcessingStateAtDepth(target, active, depth);
    }
    if (target.GetNode(depth) != RouteNodeId::Detector)
    {
        return false;
    }
    auto equalizerTarget = target;
    equalizerTarget.deviceId = DeviceId::Equalizer;
    return detectorEqualizer_.ApplyProcessingStateAtDepth(
        equalizerTarget,
        active,
        depth + 1);
}

bool Compressor::ApplyMonitoringState(
    const core::StatePath& target,
    bool enabled,
    std::size_t depth)
{
    if (target.GetDeviceId() != GetDeviceId() ||
        depth + 1 != target.GetDepth() ||
        target.GetNode(depth) != RouteNodeId::Detector)
    {
        return false;
    }

    detectorListen_ = enabled;
    return true;
}

bool Compressor::StageRuntimeUpdate(
    const core::StatePath& route,
    const ParameterVariant& value)
{
    return ApplyParameter(route, value, 0);
}

void Compressor::CommitRuntimeUpdates()
{
    detectorEqualizer_.CommitRuntimeUpdates();
    RecalculateRuntime();
}

void Compressor::SetThreshold(float thresholdDb) noexcept
{
    runtimeState_.thresholdDb = thresholdDb;
}

void Compressor::SetRatio(float ratio) noexcept
{
    runtimeState_.ratio = ratio;
}

void Compressor::SetAttack(float attackMs) noexcept
{
    runtimeState_.attackMs = attackMs;

    RecalculateAttackCoefficient();
}

void Compressor::SetRelease(float releaseMs) noexcept
{
    runtimeState_.releaseMs = releaseMs;

    RecalculateReleaseCoefficient();
}

void Compressor::SetOutputDb(float outputDb)
{
    runtimeState_.outputDb = outputDb;

    RecalculateOutputGain();
}

void Compressor::SetMix(float mix) noexcept
{
    runtimeState_.mix = mix;
    RecalculateMix();
}

void Compressor::RecalculateRuntime()
{
    RecalculateAttackCoefficient();
    RecalculateReleaseCoefficient();
    RecalculateOutputGain();
    RecalculateMix();

    runtimeState_.isNeutral = runtimeState_.thresholdDb >= 0.0f
            && runtimeState_.ratio <= 1.0f
            && runtimeState_.outputDb == 0.0f;
}

void Compressor::RecalculateAttackCoefficient() noexcept
{
    runtimeState_.attackCoefficient = CalculateTimeCoefficient(
        runtimeState_.attackMs,
        runtimeState_.sampleRate);
}

void Compressor::RecalculateReleaseCoefficient() noexcept
{
    runtimeState_.releaseCoefficient = CalculateTimeCoefficient(
        runtimeState_.releaseMs,
        runtimeState_.sampleRate);
}

void Compressor::RecalculateOutputGain()
{
    runtimeState_.outputGainLinear = std::pow(10.0, static_cast<double>(runtimeState_.outputDb) / 20.0);
}

void Compressor::RecalculateMix() noexcept
{
    runtimeState_.wetMix = static_cast<double>(runtimeState_.mix);
    runtimeState_.dryMix = 1.0 - runtimeState_.wetMix;
}

} // namespace consolidator::dsp
