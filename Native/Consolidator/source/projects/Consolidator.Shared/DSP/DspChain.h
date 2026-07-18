#pragma once

#include "../Audio/AudioBlockView.h"
#include "IDspDevice.h"

#include <cstddef>
#include <memory>
#include <utility>
#include <vector>

namespace consolidator::dsp {

class DspChain final : public IDspDevice {
public:
    void AddDevice(std::unique_ptr<IDspDevice> device) {
        if (device) {
            devices.push_back(std::move(device));
        }
    }

    void Clear() {
        devices.clear();
    }

    double ProcessSample(double input) override {
        for (const auto& device : devices) {
            input = device->ProcessSample(input);
        }
        return input;
    }

    void ProcessBlock(std::span<double> samples) override {
        for (const auto& device : devices) {
            device->ProcessBlock(samples);
        }
    }

    void Process(audio::AudioBlockView block) {
        ProcessBlock(block.Samples());
    }

    void Reset() override {
        for (const auto& device : devices) {
            device->Reset();
        }
    }

    std::size_t DeviceCount() const {
        return devices.size();
    }

private:
    std::vector<std::unique_ptr<IDspDevice>> devices;
};

} // namespace consolidator::dsp
