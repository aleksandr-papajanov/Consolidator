#include "EqualizerDevice.h"

#include <cstddef>

namespace consolidator::audio::devices
{

void EqualizerDevice::Prepare(const ProcessingContext& context) noexcept
{
    for (auto& filter : filters_)
    {
        filter.Prepare(context);
    }
}

void EqualizerDevice::Reset() noexcept
{
    for (auto& filter : filters_)
    {
        filter.Reset();
    }
}

void EqualizerDevice::SetParameters(const EqualizerParameters& parameters) noexcept
{
    for (std::size_t index = 0; index < filters_.size(); ++index)
    {
        filters_[index].SetParameters(parameters.filters[index],
            !parameters.bypass && parameters.banks[index / 7]);
    }
}

void EqualizerDevice::Process(AudioBuffer audio, ChannelMode mode) noexcept
{
    // Bank order and filter order are explicit: all 49 slots form one serial cascade.
    for (auto& filter : filters_)
    {
        filter.Process(audio, mode);
    }
}

} // namespace consolidator::audio::devices
