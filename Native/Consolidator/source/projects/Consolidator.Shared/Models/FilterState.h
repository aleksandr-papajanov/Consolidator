#pragma once

#include <optional>
#include <vector>

namespace consolidator::models {

struct FilterState {
    long filterId = 0;
    std::vector<double> values;
    bool bypass = false;
    std::optional<long> bankIndex;
};

} // namespace consolidator::models
