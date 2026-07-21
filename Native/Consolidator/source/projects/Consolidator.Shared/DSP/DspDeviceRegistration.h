#pragma once

#include "IDspDeviceFactory.h"

#include <cstddef>
#include <memory>
#include <string>

namespace consolidator::dsp {

struct DspDeviceRegistration {
    std::string deviceId;
    std::shared_ptr<const IDspDeviceFactory> factory;
    bool bypassed = false;
    long order = 0;
    std::size_t smoothingSamples = 1;
};

} // namespace consolidator::dsp
