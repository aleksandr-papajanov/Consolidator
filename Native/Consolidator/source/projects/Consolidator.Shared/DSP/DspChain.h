#pragma once

#include "../Audio/AudioBlockView.h"
#include "DspDeviceRegistration.h"
#include "IDspDevice.h"
#include "SmoothedParameter.h"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace consolidator::dsp {

class DspChain final : public IDspDevice {
public:
    void AddDevice(const DspDeviceRegistration& registration) {
        if (!registration.factory) return;
        auto device = registration.factory->Create();
        if (!device) return;
        devices.push_back(std::make_unique<DeviceSlot>(registration, std::move(device)));
    }

    void Clear() {
        devices.clear();
    }

    double ProcessSample(double input) override {
        return ProcessSampleObserved(input,
            [](std::size_t, double, double, const DspDeviceTelemetry&) {});
    }

    template <typename Observer>
    double ProcessSampleObserved(double input, Observer&& observer) {
        std::size_t index = 0;
        for (const auto& slot : devices) {
            const auto deviceInput = input;
            const auto processed = slot->device->ProcessSample(input);
            const auto bypass = slot->bypass.Next().value;
            input = processed + (input - processed) * bypass;
            observer(index++, deviceInput, input, slot->device->Telemetry());
        }
        return input;
    }

    void ProcessBlock(std::span<double> samples) override {
        IDspDevice::ProcessBlock(samples);
    }

    void Process(audio::AudioBlockView block) {
        ProcessBlock(block.Samples());
    }

    void Reset() override {
        for (const auto& slot : devices) {
            slot->device->Reset();
        }
    }

    std::size_t DeviceCount() const {
        return devices.size();
    }

    bool CanUpdate(const std::vector<DspDeviceRegistration>& registrations) const {
        if (registrations.size() != devices.size()) return false;
        for (std::size_t index = 0; index < devices.size(); ++index) {
            const auto& registration = registrations[index];
            const auto& slot = *devices[index];
            if (slot.deviceId != registration.deviceId || slot.order != registration.order ||
                !registration.factory || !registration.factory->CanUpdate(*slot.device)) {
                return false;
            }
        }
        return true;
    }

    bool Update(const std::vector<DspDeviceRegistration>& registrations) {
        if (!CanUpdate(registrations)) return false;
        for (std::size_t index = 0; index < devices.size(); ++index) {
            const auto& registration = registrations[index];
            auto& slot = *devices[index];
            registration.factory->Update(*slot.device);
            slot.bypass.SetTarget(registration.bypassed ? 1.0 : 0.0);
        }
        return true;
    }

private:
    struct DeviceSlot {
        DeviceSlot(const DspDeviceRegistration& registration, std::unique_ptr<IDspDevice> device)
            : deviceId(registration.deviceId), order(registration.order),
              device(std::move(device)),
              bypass(registration.bypassed ? 1.0 : 0.0, registration.smoothingSamples) {}

        std::string deviceId;
        long order;
        std::unique_ptr<IDspDevice> device;
        SmoothedParameter bypass;
    };

    std::vector<std::unique_ptr<DeviceSlot>> devices;
};

} // namespace consolidator::dsp
