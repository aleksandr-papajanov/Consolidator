#pragma once

#include <vector>

namespace consolidator::models {

struct FilterState {
    long filterId = 0;
    std::vector<double> values;
    bool bypass = false;

    bool operator==(const FilterState&) const = default;
};

} // namespace consolidator::models
