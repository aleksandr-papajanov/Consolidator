#pragma once

#include "FilterOptions.h"

#include <string_view>

namespace consolidator::settings {

class DetectorFilterOptions final {
public:
    static const models::FilterDefinition& Definition(long filterId = 7) {
        return settings::FilterOptions::Definitions().at(filterId);
    }

    static double DefaultValue(std::string_view name) {
        for (const auto& parameter : Definition().parameters) {
            if (parameter.name == name) return parameter.defaultValue;
        }
        return 0.0;
    }
};

} // namespace consolidator::settings
