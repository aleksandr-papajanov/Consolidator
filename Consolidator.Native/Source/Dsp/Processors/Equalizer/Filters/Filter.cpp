#include "Dsp/Processors/Equalizer/Filters/Filter.h"

#include <algorithm>
#include <cmath>


namespace consolidator::dsp
{

void Filter::ReadState(
    const core::StatePath& path,
    core::StateSnapshot& snapshot,
    DeviceId deviceId,
    RouteNodeId parentNode) const
{
    const auto filterNode = static_cast<RouteNodeId>(
        static_cast<std::uint8_t>(RouteNodeId::Filter1) + GetElementIndex());
    AppendParameter(path, snapshot, core::StatePath{deviceId, ParameterId::Frequency, parentNode, filterNode}, state_.frequencyHz);
    AppendParameter(path, snapshot, core::StatePath{deviceId, ParameterId::Q, parentNode, filterNode}, state_.q);
    AppendParameter(path, snapshot, core::StatePath{deviceId, ParameterId::Gain, parentNode, filterNode}, state_.gainDb);
    AppendParameter(path, snapshot, core::StatePath{deviceId, ParameterId::Bypass, parentNode, filterNode}, state_.bypass);
}

void Filter::ReadState(const core::StatePath& path, core::StateSnapshot& snapshot) const
{
    ReadState(path, snapshot, DeviceId::Equalizer, RouteNodeId::Detector);
}

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

bool Filter::WriteOwnParameter(
    const core::StatePath& route,
    const ParameterValue& value)
{
    const bool isUpdated =
        state_.frequencyHz.Apply(route, value) ||
        state_.q.Apply(route, value) ||
        state_.gainDb.Apply(route, value) ||
        state_.bypass.Apply(route, value);

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

double Filter::ProcessSample(double input, std::size_t channel) noexcept
{
    if (channel >= runtimeState_.channelStates.size())
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
    const auto& coefficients = runtimeState_.coefficients;

    return state_.bypass ||
           (coefficients.b0 == 1.0 &&
            coefficients.b1 == 0.0 &&
            coefficients.b2 == 0.0 &&
            coefficients.a1 == 0.0 &&
            coefficients.a2 == 0.0);
}

double Filter::GetMaximumFrequencyHz() const noexcept
{
    return runtimeState_.sampleRate * 0.49;
}

} // namespace consolidator::dsp
