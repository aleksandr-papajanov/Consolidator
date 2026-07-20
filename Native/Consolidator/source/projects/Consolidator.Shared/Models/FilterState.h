#pragma once

#include <vector>

namespace consolidator::models {

struct FilterState {
    long filterId = 0;
    std::vector<double> values;
    bool bypass = false;
};

} // namespace consolidator::models
