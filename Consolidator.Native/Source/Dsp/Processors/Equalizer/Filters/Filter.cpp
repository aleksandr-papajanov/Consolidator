#include "Dsp/Processors/Equalizer/Filters/Filter.h"

#include <algorithm>
#include <cmath>


namespace consolidator::dsp
{

Filter::Filter(
    DeviceId deviceId,
    detail::ElementKind elementKind,
    std::uint8_t elementIndex)
    : DspDevice(deviceId, elementKind, elementIndex)
{
}

void Filter::Prepare(double sampleRate, std::size_t channelCount)
{
    runtimeState_.sampleRate = std::max(sampleRate, 1.0);
    runtimeState_.activeChannelCount = std::clamp<std::size_t>(
        channelCount,
        1,
        kMaximumChannelCount);

    RecalculateCoefficients();
    RecalculateRuntime();
}

void Filter::Reset() noexcept
{
    for (auto& channelState : runtimeState_.channelStates)
    {
        channelState.z1 = 0.0;
        channelState.z2 = 0.0;
    }
}

void Filter::Process(
    const double* input,
    double* output,
    std::size_t frameCount,
    std::size_t channelCount)
{
    const std::size_t activeChannels = std::min(channelCount, runtimeState_.activeChannelCount);

    for (std::size_t frame = 0; frame < frameCount; ++frame)
    {
        const auto frameOffset = frame * channelCount;
        for (std::size_t channel = 0; channel < activeChannels; ++channel)
        {
            const auto sampleIndex = frameOffset + channel;
            output[sampleIndex] = ProcessSample(input[sampleIndex], channel);
        }
    }
}

bool Filter::ApplyOwnParameter(
    const core::StatePath& route,
    const ParameterVariant& value)
{
    bool isUpdated = false;
    if (route.GetParameterId() == ParameterId::Frequency)
    {
        const auto* v = std::get_if<float>(&value);
        if (v == nullptr) return false;
        runtimeState_.frequencyHz = *v;
        isUpdated = true;
    }
    else if (route.GetParameterId() == ParameterId::Q)
    {
        const auto* v = std::get_if<float>(&value);
        if (v == nullptr) return false;
        runtimeState_.q = *v;
        isUpdated = true;
    }
    else if (route.GetParameterId() == ParameterId::Gain)
    {
        const auto* v = std::get_if<float>(&value);
        if (v == nullptr) return false;
        runtimeState_.gainDb = *v;
        isUpdated = true;
    }

    if (isUpdated)
    {
        RecalculateCoefficients();
    }

    return isUpdated;
}

bool Filter::IsNeutral() const noexcept
{
    return runtimeState_.isNeutral;
}

void Filter::CommitRuntimeUpdates()
{
    RecalculateRuntime();
}

double Filter::ProcessSample(double input, std::size_t channel) noexcept
{
    if (!IsActive() || IsNeutral() || channel >= runtimeState_.channelStates.size())
    {
        return input;
    }

    return ProcessActiveSample(input, channel);
}

double Filter::ProcessActiveSample(double input, std::size_t channel) noexcept
{
    auto& memory = runtimeState_.channelStates[channel];
    const double output = runtimeState_.coefficients.b0 * input + memory.z1;

    memory.z1 = runtimeState_.coefficients.b1 * input - runtimeState_.coefficients.a1 * output + memory.z2;
    memory.z2 = runtimeState_.coefficients.b2 * input - runtimeState_.coefficients.a2 * output;

    return output;
}

void Filter::RecalculateRuntime() noexcept
{
    runtimeState_.isNeutral = CalculateIsNeutral();
}

bool Filter::CalculateIsNeutral() const noexcept
{
    return runtimeState_.gainDb == 0.0f;
}

double Filter::GetMaximumFrequencyHz() const noexcept
{
    return runtimeState_.sampleRate * 0.49;
}

} // namespace consolidator::dsp
