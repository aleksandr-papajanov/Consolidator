#include "Dsp/Processors/DspChain.h"

#include <algorithm>
#include <cassert>

namespace consolidator::dsp
{

void DspChain::AddDevice(std::unique_ptr<DspDevice> device)
{
    assert(devices_.size() < kMaximumDevices);
    devices_.push_back(std::move(device));
}

void DspChain::ApplyRuntimeUpdates(const core::DspStateBatch& batch)
{
    std::array<bool, kMaximumDevices> dirtyDevices{};
    for (std::size_t updateIndex = 0; updateIndex < batch.count; ++updateIndex)
    {
        const auto& update = batch.updates[updateIndex];
        for (std::size_t deviceIndex = 0;
             deviceIndex < devices_.size();
             ++deviceIndex)
        {
            if (devices_[deviceIndex]->StageRuntimeUpdate(update.path, update.value))
            {
                dirtyDevices[deviceIndex] = true;
                break;
            }
        }
    }
    for (std::size_t deviceIndex = 0;
         deviceIndex < devices_.size();
         ++deviceIndex)
    {
        if (dirtyDevices[deviceIndex])
        {
            devices_[deviceIndex]->CommitRuntimeUpdates();
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
