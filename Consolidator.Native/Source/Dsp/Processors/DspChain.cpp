#include "Dsp/Processors/DspChain.h"

#include <algorithm>

namespace consolidator::dsp
{

void DspChain::AddDevice(std::unique_ptr<DspDevice> device)
{
    devices_.push_back(std::move(device));
}

void DspChain::ApplyParameterChange(const RoutedParameterChange& change)
{
    for (auto& device : devices_)
    {
        if (device->GetDeviceId() == change.route.GetDeviceId())
        {
            device->ApplyParameter(change.route, change.value, 0);
        }
    }
}

void DspChain::AppendState(const core::StatePath& path, core::StateSnapshot& snapshot) const
{
    for (const auto& device : devices_)
    {
        device->AppendState(path, snapshot);
    }
}

void DspChain::Process(
    const double* input,
    double* interim,
    double* output,
    std::size_t frameCount,
    std::size_t channelCount)
{
    const auto sampleCount = frameCount * channelCount;
    const double* source = input;
    bool hasProcessed = false;
    bool outputContainsResult = false;

    for (const auto& device : devices_)
    {
        if (device->IsNeutral())
        {
            continue;
        }

        if (!hasProcessed)
        {
            device->Process(source, output, frameCount, channelCount);
            source = output;
            hasProcessed = true;
            outputContainsResult = true;
        }
        else
        {
            double* destination = outputContainsResult ? interim : output;
            device->Process(source, destination, frameCount, channelCount);
            source = destination;
            outputContainsResult = !outputContainsResult;
        }
    }

    if (!hasProcessed)
    {
        std::copy_n(input, sampleCount, output);
    }
    else if (!outputContainsResult)
    {
        std::copy_n(source, sampleCount, output);
    }
}

std::size_t DspChain::GetDeviceCount() const noexcept
{
    return devices_.size();
}

DspDevice* DspChain::GetDevice(std::size_t index) noexcept
{
    return index < devices_.size() ? devices_[index].get() : nullptr;
}

const DspDevice* DspChain::GetDevice(std::size_t index) const noexcept
{
    return index < devices_.size() ? devices_[index].get() : nullptr;
}

} // namespace consolidator::dsp
