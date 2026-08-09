#include "Dsp/Processors/DspChain.h"

#include <algorithm>

namespace consolidator::dsp
{

void DspChain::AddDevice(std::unique_ptr<DspDevice> device)
{
    devices_.push_back(std::move(device));
}

void DspChain::ReadState(const core::StatePath& path, core::StateResponseEntries& snapshot) const
{
    for (const auto& device : devices_)
    {
        device->ReadState(path, snapshot);
    }
}

core::StateWriteStatus DspChain::WriteState(const core::StateEntry& entry, core::StateResponseEntries& applied)
{
    if (!entry.path.deviceId)
    {
        return core::StateWriteStatus::NotHandled;
    }

    for (const auto& device : devices_)
    {
        if (device->GetDeviceId() != *entry.path.deviceId)
        {
            continue;
        }
        const auto status = device->WriteState(entry, applied);
        if (status != core::StateWriteStatus::NotHandled)
        {
            return status;
        }
    }
    return core::StateWriteStatus::NotHandled;
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
