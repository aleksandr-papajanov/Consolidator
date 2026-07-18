#pragma once

#include "IDspDevice.h"

#include <memory>

namespace consolidator::dsp {

class IDspDeviceFactory {
public:
    virtual ~IDspDeviceFactory() = default;

    virtual std::unique_ptr<IDspDevice> Create() const = 0;
};

} // namespace consolidator::dsp
