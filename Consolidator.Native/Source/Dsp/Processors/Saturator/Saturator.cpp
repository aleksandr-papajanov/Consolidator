#include "Dsp/Processors/Saturator/Saturator.h"

#include <algorithm>
#include <cassert>
#include <cmath>


namespace consolidator::dsp
{

Saturator::Saturator()
    : DspDevice(DeviceId::Saturator, detail::ElementKind::Device, 0)
{
    RecalculateRuntime();
}

void Saturator::Prepare(
    double sampleRate,
    std::size_t channelCount)
{
    assert(channelCount > 0);
    assert(channelCount <= kMaximumChannelCount);

    activeChannelCount_ = std::clamp<std::size_t>(
        channelCount,
        1,
        kMaximumChannelCount);

    for (std::size_t channel = 0; channel < activeChannelCount_; ++channel)
    {
        detectors_[channel].Prepare(sampleRate);
    }

    Reset();
}

void Saturator::Reset() noexcept
{
    for (auto& detector : detectors_)
    {
        detector.Reset();
    }
}

bool Saturator::Reset(
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

    for (auto& detector : detectors_)
    {
        if (detector.Reset(route, depth + 1))
        {
            return true;
        }
    }
    return false;
}

void Saturator::Process(
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

    for (std::size_t frame = 0; frame < frameCount; ++frame)
    {
        outputLeft[frame] = activeChannelCount_ > 0
            ? ProcessSample(inputLeft[frame], detectors_[0]) : inputLeft[frame];
        outputRight[frame] = activeChannelCount_ > 1
            ? ProcessSample(inputRight[frame], detectors_[1]) : inputRight[frame];
    }
}

double Saturator::ProcessSample(double input, DetectorEnvelopeFollower& detector) const noexcept
{
    const double envelope = detector.ProcessSample(input);
    if (detector.IsListening())
    {
        return detector.GetMonitoringSample();
    }
    const double modulation = CalculateDriveModulation(envelope);
    const double effectiveDrive = runtimeState_.driveLinear * modulation;
    const double saturated = ApplyWaveshaper(input, effectiveDrive);
    const double wet = saturated * runtimeState_.outputGainLinear;

    return
        wet * runtimeState_.wetMix +
        input * runtimeState_.dryMix;
}

double Saturator::CalculateDriveModulation(double envelope) const noexcept
{
    const double modulation = 1.0 + envelope * runtimeState_.detectorAmount;

    return std::clamp(
        modulation,
        1.0,
        core::settings::SaturatorDefaults::kMaximumDriveModulation);
}

double Saturator::ApplyWaveshaper(double input, double drive) const noexcept
{
    const double safeDrive = std::max(
        drive,
        core::settings::SaturatorDefaults::kMinDrive);
    const double normalization = std::tanh(safeDrive);

    if (normalization <= 0.0)
    {
        return input;
    }

    return std::tanh(input * safeDrive) / normalization;
}

bool Saturator::ApplyOwnParameter(
    const core::StatePath& route,
    const ParameterVariant& value)
{
    if (route.GetParameterId() == ParameterId::Drive)
    {
        const auto* updated = std::get_if<float>(&value);
        if (updated == nullptr) return false;
        runtimeState_.drive = *updated;
        return true;
    }
    if (route.GetParameterId() == ParameterId::Gain)
    {
        const auto* updated = std::get_if<float>(&value);
        if (updated == nullptr) return false;
        runtimeState_.outputDb = *updated;
        return true;
    }
    if (route.GetParameterId() == ParameterId::Mix)
    {
        const auto* updated = std::get_if<float>(&value);
        if (updated == nullptr) return false;
        runtimeState_.mix = *updated;
        return true;
    }
    if (route.GetParameterId() == ParameterId::DetectorAmount)
    {
        const auto* updated = std::get_if<float>(&value);
        if (updated == nullptr) return false;
        runtimeState_.detectorAmountTarget = *updated;
        return true;
    }
    return false;
}

bool Saturator::ApplyParameter(
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

    bool isUpdated = false;
    for (auto& detector : detectors_)
    {
        isUpdated = detector.ApplyParameter(route, value, depth + 1) || isUpdated;
    }

    return isUpdated;
}

bool Saturator::ApplyProcessingStateAtDepth(
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
    bool applied = false;
    for (auto& detector : detectors_)
    {
        applied = detector.ApplyProcessingStateAtDepth(
            target, active, depth + 1) || applied;
    }
    return applied;
}

bool Saturator::ApplyMonitoringState(
    const core::StatePath& target,
    bool enabled,
    std::size_t depth)
{
    if (target.GetDeviceId() != GetDeviceId() ||
        depth >= target.GetDepth() ||
        target.GetNode(depth) != RouteNodeId::Detector)
    {
        return false;
    }

    bool applied = false;
    for (auto& detector : detectors_)
    {
        applied = detector.ApplyMonitoringState(target, enabled, depth + 1) || applied;
    }
    return applied;
}

bool Saturator::StageRuntimeUpdate(
    const core::StatePath& route,
    const ParameterVariant& value)
{
    return ApplyParameter(route, value, 0);
}

void Saturator::CommitRuntimeUpdates()
{
    for (auto& detector : detectors_)
    {
        detector.CommitRuntimeUpdates();
    }
    RecalculateRuntime();
}

void Saturator::SetDrive(float drive)
{
    runtimeState_.drive = drive;
    runtimeState_.driveLinear = static_cast<double>(runtimeState_.drive);
}

void Saturator::SetOutputDb(float outputDb)
{
    runtimeState_.outputDb = outputDb;

    runtimeState_.outputGainLinear = std::pow(10.0, static_cast<double>(outputDb) / 20.0);
}

void Saturator::SetMix(float mix)
{
    runtimeState_.mix = mix;
    runtimeState_.wetMix = static_cast<double>(runtimeState_.mix);

    runtimeState_.dryMix = 1.0 - runtimeState_.wetMix;
}

void Saturator::SetDetectorAmount(float amount)
{
    runtimeState_.detectorAmountTarget = amount;
    runtimeState_.detectorAmount = static_cast<double>(runtimeState_.detectorAmountTarget);
}

void Saturator::RecalculateRuntime()
{
    SetDrive(runtimeState_.drive);
    SetOutputDb(runtimeState_.outputDb);
    SetMix(runtimeState_.mix);
    SetDetectorAmount(runtimeState_.detectorAmountTarget);
    runtimeState_.isNeutral = runtimeState_.driveLinear == 1.0
            && runtimeState_.outputGainLinear == 1.0
            && runtimeState_.wetMix == 1.0;
}

} // namespace consolidator::dsp
