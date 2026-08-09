#include "Dsp/Processors/Gain/Gain.h"

#include <algorithm>
#include <cmath>


namespace consolidator::dsp
{

void Gain::RecalculateRuntime()
{
    runtimeState_.linearGain = std::pow(10.0, static_cast<double>(state_.gainDb) / 20.0);
    runtimeState_.isNeutral = state_.bypass || runtimeState_.linearGain == 1.0;
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
