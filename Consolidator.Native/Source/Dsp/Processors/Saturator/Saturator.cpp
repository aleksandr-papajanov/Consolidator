#include "Dsp/Processors/Saturator/Saturator.h"

#include <algorithm>
#include <cassert>
#include <cmath>

#include "Dsp/Parameters/ParameterHelper.h"

namespace consolidator::dsp
{

Saturator::Saturator()
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

    if (state_.bypass || (runtime_.driveLinear == 1.0 && runtime_.outputGainLinear == 1.0 && runtime_.wetMix == 1.0))
    {
        std::copy_n(input, sampleCount, output);
        return;
    }

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

double Saturator::ProcessSample(double input, EnvelopeDetector& detector) const noexcept
{
    const double envelope = detector.ProcessSample(input);
    const double modulation = CalculateDriveModulation(envelope);
    const double effectiveDrive = runtime_.driveLinear * modulation;
    const double saturated = ApplyWaveshaper(input, effectiveDrive);
    const double wet = saturated * runtime_.outputGainLinear;

    return
        wet * runtime_.wetMix +
        input * runtime_.dryMix;
}

double Saturator::CalculateDriveModulation(double envelope) const noexcept
{
    const double modulation = 1.0 + envelope * runtime_.detectorAmount;

    return std::clamp(
        modulation,
        1.0,
        kMaximumDriveModulation);
}

double Saturator::ApplyWaveshaper(double input, double drive) const noexcept
{
    const double safeDrive =  std::max(drive, static_cast<double>(kMinimumDrive));
    const double normalization = std::tanh(safeDrive);

    if (normalization <= 0.0)
    {
        return input;
    }

    return std::tanh(input * safeDrive) / normalization;
}

void Saturator::ApplyParameterChange(const ParameterChange& change)
{
    if (change.address.GetElementKind() != detail::ElementKind::Device)
    {
        ApplyDetectorParameter(change);
        return;
    }

    ApplyDeviceParameter(change);
}

void Saturator::ApplyDeviceParameter(const ParameterChange& change)
{
    switch (change.address.GetParameterId())
    {
    case ParameterId::Drive:
        if (const auto* value = TryGetValue<float>(change))
        {
            SetDrive(*value);
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

void Saturator::ApplyDetectorParameter(const ParameterChange& change)
{
    for (auto& detector : detectors_)
    {
        detector.ApplyParameterChange(change);
    }
}

void Saturator::SetDrive(float drive)
{
    state_.drive = std::clamp(
        drive,
        kMinimumDrive,
        kMaximumDrive);

    runtime_.driveLinear = static_cast<double>(state_.drive);
}

void Saturator::SetOutputDb(float outputDb)
{
    state_.outputDb = outputDb;

    runtime_.outputGainLinear = std::pow(10.0, static_cast<double>(outputDb) / 20.0);
}

void Saturator::SetMix(float mix)
{
    state_.mix = std::clamp(
        mix,
        kMinimumMix,
        kMaximumMix);

    runtime_.wetMix = static_cast<double>(state_.mix);

    runtime_.dryMix = 1.0 - runtime_.wetMix;
}

void Saturator::SetDetectorAmount(float amount)
{
    state_.detectorAmount = std::clamp(
        amount,
        kMinimumDetectorAmount,
        kMaximumDetectorAmount);

    runtime_.detectorAmount = static_cast<double>(state_.detectorAmount);
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
}

} // namespace consolidator::dsp