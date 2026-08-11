#include "Dsp/Processors/Gain/Gain.h"

#include <algorithm>
#include <cmath>


namespace consolidator::dsp
{

bool Gain::StageRuntimeUpdate(
    const core::StatePath& path,
    const ParameterVariant& value)
{
    return DspDevice::ApplyParameter(path, value, 0);
}

bool Gain::ApplyOwnParameter(
    const core::StatePath& path,
    const ParameterVariant& value)
{
    if (path.GetParameterId() == ParameterId::Gain)
    {
        const auto* gainDb = std::get_if<float>(&value);
        if (gainDb == nullptr)
        {
            return false;
        }
        runtimeState_.gainDb = *gainDb;
        return true;
    }

    return false;
}

void Gain::RecalculateRuntime()
{
    runtimeState_.linearGain = std::pow(10.0, static_cast<double>(runtimeState_.gainDb) / 20.0);
    runtimeState_.isNeutral = runtimeState_.linearGain == 1.0;
}

void Gain::Process(
    const double* input,
    double* output,
    std::size_t frameCount,
    std::size_t channelCount)
{
    const auto sampleCount = frameCount * channelCount;

    for (std::size_t sample = 0; sample < sampleCount; ++sample)
    {
        output[sample] = input[sample] * runtimeState_.linearGain;
    }
}

} // namespace consolidator::dsp
