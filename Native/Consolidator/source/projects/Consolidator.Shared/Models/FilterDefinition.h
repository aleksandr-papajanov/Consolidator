#pragma once

#include "FilterType.h"
#include "ParameterRange.h"

#include <array>
#include <optional>
#include <string>
#include <vector>

namespace consolidator::models {

struct FilterParameterDefinition {
    std::string name;
    ParameterRange range;
    double defaultValue = 0.0;
};

struct FilterDefinition {
    long filterId = 0;
    FilterType type = FilterType::Peak;
    std::vector<FilterParameterDefinition> parameters;
    bool defaultBypass = false;
    std::array<double, 4> color{ 1.0, 1.0, 1.0, 1.0 };

    std::vector<double> DefaultValues() const {
        std::vector<double> values;
        values.reserve(parameters.size());
        for (const auto& parameter : parameters) values.push_back(parameter.defaultValue);
        return values;
    }

    std::optional<std::size_t> ParameterIndex(const std::string& name) const {
        for (std::size_t index = 0; index < parameters.size(); ++index) {
            if (parameters[index].name == name) return index;
        }
        return std::nullopt;
    }

    const FilterParameterDefinition* FindParameter(const std::string& name) const {
        const auto index = ParameterIndex(name);
        return index ? &parameters[*index] : nullptr;
    }

    double Value(const std::vector<double>& values, const std::string& name, double fallback) const {
        const auto index = ParameterIndex(name);
        return index && *index < values.size() ? values[*index] : fallback;
    }
};

} // namespace consolidator::models
