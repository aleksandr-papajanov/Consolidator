#pragma once

#include "EqSnapshot.h"
#include "FilterDefinition.h"

#include <vector>

namespace consolidator::models {

struct DeviceState {
    long revision = 0;
    long generation = 0;
    std::vector<FilterDefinition> filterDefinitions;
    EqSnapshot snapshot;
};

} // namespace consolidator::models
