#pragma once

#include "EqSnapshot.h"

namespace consolidator::models {

struct DeviceState {
    long revision = 0;
    long generation = 0;
    EqSnapshot snapshot;
};

} // namespace consolidator::models
