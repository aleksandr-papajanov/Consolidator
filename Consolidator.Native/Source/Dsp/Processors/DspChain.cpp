#include "Dsp/Processors/DspChain.h"

namespace consolidator::dsp
{

void DspChain::AddDevice(std::unique_ptr<IDspDevice> device)
{
    devices_.push_back(std::move(device));
}

void DspChain::ApplyParameterChange(const ParameterChange& change)
{
    pendingChanges_.push_back(change);
}

void DspChain::ApplyParameterBatch(const ParameterBatch& batch)
{
    for (const auto& change : batch.GetChanges())
    {
        pendingChanges_.push_back(change);
    }
}

void DspChain::Process(
    const double* input,
    double* interim,
    double* output,
    std::size_t frameCount,
    std::size_t channelCount)
{
    ApplyPendingChanges();

    const auto sampleCount = frameCount * channelCount;

    const double* src = input;
    double* dst = output;
    bool hasProcessed = false;
    std::size_t lastActiveIndex = 0;

    for (std::size_t i = 0; i < devices_.size(); ++i)
    {
        if (devices_[i]->IsNeutral())
        {
            continue;
        }

        lastActiveIndex = i;

        if (!hasProcessed)
        {
            devices_[i]->Process(src, interim, frameCount, channelCount);
            src = interim;
            hasProcessed = true;
        }
        else
        {
            devices_[i]->Process(src, interim, frameCount, channelCount);
            src = interim;
        }
    }

    if (!hasProcessed)
    {
        std::copy_n(input, sampleCount, output);
        return;
    }

    std::copy_n(src, sampleCount, output);
}

std::size_t DspChain::GetDeviceCount() const noexcept
{
    return devices_.size();
}

IDspDevice* DspChain::GetDevice(std::size_t index) noexcept
{
    return index < devices_.size() ? devices_[index].get() : nullptr;
}

const IDspDevice* DspChain::GetDevice(std::size_t index) const noexcept
{
    return index < devices_.size() ? devices_[index].get() : nullptr;
}

void DspChain::ApplyPendingChanges()
{
    for (const auto& change : pendingChanges_)
    {
        ApplyChangeToDevice(change);
    }
    pendingChanges_.clear();
}

void DspChain::ApplyChangeToDevice(const ParameterChange& change)
{
    for (auto& device : devices_)
    {
        if (device->GetDeviceId() == change.address.GetDeviceId())
        {
            device->ApplyParameterChange(change);
        }
    }
}

} // namespace consolidator::dsp