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

void Saturator::Process(
    const double* input,
    double* output,
    std::size_t frameCount,
    std::size_t channelCount)
{
    assert(input != nullptr);
    assert(output != nullptr);

    const auto sampleCount = frameCount * channelCount;

    for (std::size_t frame = 0; frame < frameCount; ++frame)
    {
        for (std::size_t channel = 0;  channel < channelCount; ++channel)
        {
            const auto sampleIndex = frame * channelCount + channel;

            if (channel >= activeChannelCount_)
            {
                output[sampleIndex] = input[sampleIndex];
                continue;
            }

            output[sampleIndex] = ProcessSample(input[sampleIndex], detectors_[channel]);
        }
    }
}

double Saturator::ProcessSample(double input, DetectorEnvelopeFollower& detector) const noexcept
{
    const double envelope = detector.ProcessSample(input);
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

bool Saturator::ApplyStateParameter(
    const ParameterRoute& route,
    const ParameterValue& value)
{
    return state_.drive.Apply(route, value) ||
           state_.outputDb.Apply(route, value) ||
           state_.mix.Apply(route, value) ||
           state_.detectorAmount.Apply(route, value) ||
           state_.bypass.Apply(route, value);
}

bool Saturator::ApplyParameter(
    const ParameterRoute& route,
    const ParameterValue& value,
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

    if (isUpdated)
    {
        RecalculateRuntime();
    }

    return isUpdated;
}

void Saturator::SetDrive(float drive)
{
    state_.drive = drive;

    runtimeState_.driveLinear = static_cast<double>(state_.drive);
}

void Saturator::SetOutputDb(float outputDb)
{
    state_.outputDb = outputDb;

    runtimeState_.outputGainLinear = std::pow(10.0, static_cast<double>(outputDb) / 20.0);
}

void Saturator::SetMix(float mix)
{
    state_.mix = mix;

    runtimeState_.wetMix = static_cast<double>(state_.mix);

    runtimeState_.dryMix = 1.0 - runtimeState_.wetMix;
}

void Saturator::SetDetectorAmount(float amount)
{
    state_.detectorAmount = amount;

    runtimeState_.detectorAmount = static_cast<double>(state_.detectorAmount);
}

void Saturator::SetBypass(bool bypass) noexcept
{
    state_.bypass = bypass;
}

void Saturator::RecalculateRuntime()
{
    SetDrive(state_.drive);
    SetOutputDb(state_.outputDb);
    SetMix(state_.mix);
    SetDetectorAmount(state_.detectorAmount);
    SetBypass(state_.bypass);

    runtimeState_.isNeutral = state_.bypass
        || (runtimeState_.driveLinear == 1.0
            && runtimeState_.outputGainLinear == 1.0
            && runtimeState_.wetMix == 1.0);
}

} // namespace consolidator::dsp
