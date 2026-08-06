#include "Dsp/Processors/Gain/Gain.h"

#include <algorithm>
#include <cmath>

#include "Dsp/Parameters/ParameterHelper.h"

namespace consolidator::dsp
{

void Gain::RecalculateRuntime()
{
    runtime_.linearGain = std::pow(10.0, static_cast<double>(state_.gainDb) / 20.0);
}

void Gain::Process(
    const double* input,
    double* output,
    std::size_t frameCount,
    std::size_t channelCount)
{
    const auto sampleCount = frameCount * channelCount;

    if (state_.bypass || runtime_.linearGain == 1.0)
    {
        std::copy_n(input, sampleCount, output);
        return;
    }

    for (std::size_t sample = 0; sample < sampleCount; ++sample)
    {
        output[sample] = input[sample] * runtime_.linearGain;
    }
}

void Gain::ApplyParameterChange(const ParameterChange& change)
{
    if (change.address.GetDeviceId() != deviceId_)
    {
        return;
    }

    switch (change.address.GetParameterId())
    {
    case ParameterId::Gain:
        if (const auto* value = TryGetValue<float>(change))
        {
            state_.gainDb = *value;
            RecalculateRuntime();
        }
        break;

    case ParameterId::Bypass:
        if (const auto* value = TryGetValue<bool>(change))
        {
            state_.bypass = *value;
        }
        break;

    default:
        break;
    }
}

} // namespace consolidator::dsp