#include "Dsp/Processors/Equalizer/Filters/Filter.h"

#include <algorithm>
#include <cmath>

#include "Dsp/Parameters/ParameterHelper.h"

namespace consolidator::dsp
{

Filter::Filter(
    DeviceId deviceId,
    detail::ElementKind elementKind,
    std::uint8_t elementIndex)
    : deviceId_(deviceId),
      elementKind_(elementKind),
      elementIndex_(elementIndex)
{
}

void Filter::Prepare(double sampleRate, std::size_t channelCount)
{
    sampleRate_ = std::max(sampleRate, 1.0);

    activeChannelCount_ = std::clamp<std::size_t>(
        channelCount,
        1,
        kMaximumChannelCount);
}

void Filter::Reset() noexcept
{
    for (auto& state : channelStates_)
    {
        state.z1 = 0.0;
        state.z2 = 0.0;
    }
}

void Filter::Process(
    const double* input,
    double* output,
    std::size_t frameCount,
    std::size_t channelCount)
{
    const auto processedChannelCount =
        std::min(channelCount, activeChannelCount_);

    for (std::size_t frame = 0; frame < frameCount; ++frame)
    {
        const auto frameOffset = frame * channelCount;

        for (std::size_t channel = 0;
             channel < processedChannelCount;
             ++channel)
        {
            const auto sampleIndex = frameOffset + channel;

            output[sampleIndex] =
                ProcessSample(input[sampleIndex], channel);
        }
    }
}

void Filter::ApplyParameterChange(const ParameterChange& change)
{
    switch (change.address.GetParameterId())
    {
    case ParameterId::Frequency:
        if (const auto* value = TryGetValue<float>(change))
        {
            SetFrequency(*value);
        }
        break;

    case ParameterId::Q:
        if (const auto* value = TryGetValue<float>(change))
        {
            SetQ(*value);
        }
        break;

    case ParameterId::Gain:
        if (const auto* value = TryGetValue<float>(change))
        {
            SetGainDb(*value);
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

double Filter::ProcessSample(
    double input,
    std::size_t channel) noexcept
{
    if (parameters_.bypass ||
        channel >= channelStates_.size())
    {
        return input;
    }

    return ProcessActiveSample(input, channel);
}

double Filter::ProcessActiveSample(
    double input,
    std::size_t channel) noexcept
{
    auto& state = channelStates_[channel];

    const double output =
        coefficients_.b0 * input + state.z1;

    state.z1 =
        coefficients_.b1 * input -
        coefficients_.a1 * output +
        state.z2;

    state.z2 =
        coefficients_.b2 * input -
        coefficients_.a2 * output;

    return output;
}

void Filter::SetFrequency(float frequencyHz)
{
    parameters_.frequencyHz = std::clamp(
        static_cast<double>(frequencyHz),
        kMinimumFrequencyHz,
        sampleRate_ * 0.49);

    RecalculateCoefficients();
}

void Filter::SetQ(float q)
{
    parameters_.q = std::max(
        static_cast<double>(q),
        kMinimumQ);

    RecalculateCoefficients();
}

void Filter::SetGainDb(float gainDb)
{
    parameters_.gainDb = static_cast<double>(gainDb);
    RecalculateCoefficients();
}

void Filter::SetBypass(bool bypass) noexcept
{
    parameters_.bypass = bypass;
}

double Filter::GetMaximumFrequencyHz() const noexcept
{
    return sampleRate_ * 0.49;
}

} // namespace consolidator::dsp