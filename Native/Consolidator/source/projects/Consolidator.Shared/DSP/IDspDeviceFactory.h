#pragma once

#include "IDspDevice.h"

#include <memory>

namespace consolidator::dsp {

class IDspDeviceFactory {
public:
    virtual ~IDspDeviceFactory() = default;

    virtual std::unique_ptr<IDspDevice> Create() const = 0;
    virtual bool CanUpdate(const IDspDevice&) const { return false; }
    virtual void Update(IDspDevice&) const {}
};

} // namespace consolidator::dsp
