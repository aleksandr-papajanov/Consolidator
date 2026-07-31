#pragma once

#include "Models/FilterType.h"
#include "Settings/DetectorFilterOptions.h"

namespace consolidator::models {

struct DetectorFilterState {
    DetectorFilterState(long id = 1)
        : filterId(id)
        , gainDb(settings::DetectorFilterOptions::DefaultValue(id, "gain"))
        , frequencyHz(settings::DetectorFilterOptions::DefaultValue(id, "frequency"))
        , q(settings::DetectorFilterOptions::DefaultValue(id, "q")) {
    }

    long filterId = 1;
    FilterType type = FilterType::Peak;
    bool bypass = false;
    double gainDb = 0.0;
    double frequencyHz = 0.0;
    double q = 0.0;

    bool operator==(const DetectorFilterState&) const = default;
};

} // namespace consolidator::models
