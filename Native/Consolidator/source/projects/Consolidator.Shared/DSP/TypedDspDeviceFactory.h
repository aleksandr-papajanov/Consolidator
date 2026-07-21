#pragma once

#include "IDspDeviceFactory.h"

#include <memory>
#include <utility>

namespace consolidator::dsp {

template <typename Device, typename Settings>
class TypedDspDeviceFactory final : public IDspDeviceFactory {
public:
    explicit TypedDspDeviceFactory(Settings settings)
        : settings(std::move(settings)) {}

    std::unique_ptr<IDspDevice> Create() const override {
        return std::make_unique<Device>(settings);
    }

    bool CanUpdate(const IDspDevice& device) const override {
        return dynamic_cast<const Device*>(&device) != nullptr;
    }

    void Update(IDspDevice& device) const override {
        if (auto* typed = dynamic_cast<Device*>(&device)) typed->UpdateSettings(settings);
    }

private:
    Settings settings;
};

} // namespace consolidator::dsp
