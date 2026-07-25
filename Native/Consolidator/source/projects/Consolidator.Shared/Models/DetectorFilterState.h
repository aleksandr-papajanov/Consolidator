#pragma once

#include "Models/FilterType.h"
#include "Settings/DetectorFilterOptions.h"

namespace consolidator::models {

struct DetectorFilterState {
    long filterId = 1;
    FilterType type = FilterType::Peak;
    bool bypass = false;
    double gainDb = settings::DetectorFilterOptions::DefaultValue("gain");
    double frequencyHz = settings::DetectorFilterOptions::DefaultValue("frequency");
    double q = settings::DetectorFilterOptions::DefaultValue("q");

    bool operator==(const DetectorFilterState&) const = default;
};

} // namespace consolidator::models
