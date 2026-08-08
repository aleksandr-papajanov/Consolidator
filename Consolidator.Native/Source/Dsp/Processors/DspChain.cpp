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


void DspChain::Process(
    const double* input,
    double* interim,
    double* output,
    std::size_t frameCount,
    std::size_t channelCount)
{
    const auto sampleCount = frameCount * channelCount;

    const double* src = input;
    bool hasProcessed = false;
    bool outputContainsResult = false;

    for (std::size_t i = 0; i < devices_.size(); ++i)
    {
        if (devices_[i]->IsNeutral())
        {
            continue;
        }

        if (!hasProcessed)
        {
            devices_[i]->Process(src, output, frameCount, channelCount);
            src = output;
            hasProcessed = true;
            outputContainsResult = true;
        }
        else
        {
            double* const destination = outputContainsResult ? interim : output;
            devices_[i]->Process(src, destination, frameCount, channelCount);
            src = destination;
            outputContainsResult = !outputContainsResult;
        }
    }

    if (!hasProcessed)
    {
        std::copy_n(input, sampleCount, output);
        return;
    }

    if (!outputContainsResult)
    {
        std::copy_n(src, sampleCount, output);
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
