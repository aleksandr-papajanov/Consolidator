#pragma once

#include "IDspDeviceFactory.h"

#include <functional>
#include <memory>
#include <utility>

namespace consolidator::dsp {

class FunctionDspDeviceFactory final : public IDspDeviceFactory {
public:
    using Factory = std::function<std::unique_ptr<IDspDevice>()>;

    explicit FunctionDspDeviceFactory(Factory factory)
        : factory(std::move(factory)) {}

    std::unique_ptr<IDspDevice> Create() const override {
        return factory ? factory() : nullptr;
    }

private:
    Factory factory;
};

} // namespace consolidator::dsp
