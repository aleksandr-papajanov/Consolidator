#pragma once

#include "../Audio/StereoBufferView.h"
#include "../Audio/StereoSample.h"
#include "DspChain.h"
#include "DspDeviceRegistration.h"

#include <utility>
#include <optional>
#include <string_view>

namespace consolidator::dsp {

class StereoDspChain final {
public:
    StereoDspChain() = default;

    StereoDspChain(DspChain left, DspChain right)
        : left(std::move(left)), right(std::move(right)) {}

    audio::StereoSample ProcessSample(audio::StereoSample input) {
        return {
            left.ProcessSample(input.left),
            right.ProcessSample(input.right)
        };
    }

    template <typename Observer>
    audio::StereoSample ProcessSampleObserved(audio::StereoSample input, Observer&& observer) {
        return {
            left.ProcessSampleObserved(input.left, observer),
            right.ProcessSampleObserved(input.right, observer)
        };
    }

    template <typename Observer>
    audio::StereoSample ProcessSampleObservedStereo(audio::StereoSample input, Observer&& observer) {
        return {
            left.ProcessSampleObserved(input.left,
                [&observer](std::size_t index, double deviceInput, double output, const DspDeviceTelemetry& telemetry, bool bypassed) {
                    observer(0, index, deviceInput, output, telemetry, bypassed);
                }),
            right.ProcessSampleObserved(input.right,
                [&observer](std::size_t index, double deviceInput, double output, const DspDeviceTelemetry& telemetry, bool bypassed) {
                    observer(1, index, deviceInput, output, telemetry, bypassed);
                })
        };
    }

    void Process(audio::StereoBufferView buffer) {
        left.Process(buffer.Left());
        right.Process(buffer.Right());
    }

    void Reset() {
        left.Reset();
        right.Reset();
    }

    std::size_t DeviceCount() const {
        return left.DeviceCount();
    }

    bool Update(const std::vector<DspDeviceRegistration>& registrations) {
        if (!left.CanUpdate(registrations) || !right.CanUpdate(registrations)) return false;
        return left.Update(registrations) && right.Update(registrations);
    }

    bool UpdateDevice(
        std::string_view deviceId,
        const IDspDeviceFactory& factory,
        std::optional<bool> bypassed = std::nullopt
    ) {
        return left.UpdateDevice(deviceId, factory, bypassed) &&
            right.UpdateDevice(deviceId, factory, bypassed);
    }

private:
    DspChain left;
    DspChain right;
};

} // namespace consolidator::dsp
