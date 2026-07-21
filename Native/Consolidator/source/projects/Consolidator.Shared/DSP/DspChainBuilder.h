#pragma once

#include "DspDeviceRegistration.h"
#include "DspChain.h"
#include "StereoDspChain.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace consolidator::dsp {

class DspChainBuilder final {
public:
    void SetDevices(std::vector<DspDeviceRegistration> devices) {
        this->devices = std::move(devices);
        SortDevices();
    }

    void UpsertDevice(DspDeviceRegistration device) {
        const auto existing = Find(device.deviceId);
        if (existing == devices.end()) {
            devices.push_back(std::move(device));
        }
        else {
            *existing = std::move(device);
        }
        SortDevices();
    }

    void RemoveDevice(const std::string& deviceId) {
        devices.erase(std::remove_if(devices.begin(), devices.end(),
            [&deviceId](const auto& device) { return device.deviceId == deviceId; }), devices.end());
    }

    void Clear() {
        devices.clear();
    }

    DspChain Build() const {
        DspChain chain;
        AddDevices(chain);
        return chain;
    }

    StereoDspChain BuildStereo() const {
        DspChain left;
        DspChain right;
        AddDevices(left);
        AddDevices(right);
        return { std::move(left), std::move(right) };
    }

    std::size_t DeviceCount() const {
        return devices.size();
    }

    std::vector<DspDeviceRegistration> TakeDevices() {
        return std::move(devices);
    }

private:
    using Iterator = std::vector<DspDeviceRegistration>::iterator;

    Iterator Find(const std::string& deviceId) {
        return std::find_if(devices.begin(), devices.end(),
            [&deviceId](const auto& device) { return device.deviceId == deviceId; });
    }

    void AddDevices(DspChain& chain) const {
        for (const auto& device : devices) {
            chain.AddDevice(device);
        }
    }

    void SortDevices() {
        std::stable_sort(devices.begin(), devices.end(),
            [](const auto& left, const auto& right) { return left.order < right.order; });
    }

    std::vector<DspDeviceRegistration> devices;
};

} // namespace consolidator::dsp
