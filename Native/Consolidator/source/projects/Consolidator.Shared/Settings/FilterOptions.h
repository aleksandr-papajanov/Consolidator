#pragma once

#include "../Models/FilterDefinition.h"

#include <map>

namespace consolidator::settings {

class FilterOptions final {
public:
    static const std::map<long, models::FilterDefinition>& Definitions() {
        using models::FilterType;
        using models::ParameterScale;
        static const std::map<long, models::FilterDefinition> definitions = {
            { 1, { 1, FilterType::Tilt, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "pivot", { 40, 18000, ParameterScale::Logarithmic }, 1000 } }, false } },
            { 2, { 2, FilterType::LowShelf, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "freq", { 40, 800, ParameterScale::Logarithmic }, 200 } }, false } },
            { 3, { 3, FilterType::HighShelf, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "freq", { 1000, 18000, ParameterScale::Logarithmic }, 8000 } }, false } },
            { 4, { 4, FilterType::Peak, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "freq", { 100, 8000, ParameterScale::Logarithmic }, 1000 }, { "q", { .2, 1, ParameterScale::Logarithmic }, 1 } }, false } },
            { 5, { 5, FilterType::Peak, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "freq", { 40, 18000, ParameterScale::Logarithmic }, 3000 }, { "q", { .2, 1, ParameterScale::Logarithmic }, 1 } }, false } },
            { 6, { 6, FilterType::Peak, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "freq", { 40, 18000, ParameterScale::Logarithmic }, 500 }, { "q", { 1, 7, ParameterScale::Logarithmic }, 3 } }, false, models::FilterScope::Eq } },
            { 7, { 7, FilterType::Peak, { { "gain", { -24, 24, ParameterScale::Linear }, 0 }, { "frequency", { 20, 20000, ParameterScale::Logarithmic }, 1000 }, { "q", { .2, 8, ParameterScale::Logarithmic }, .707 } }, false, models::FilterScope::Detector } },
            { 8, { 8, FilterType::Peak, { { "gain", { -24, 24, ParameterScale::Linear }, 0 }, { "frequency", { 20, 20000, ParameterScale::Logarithmic }, 1000 }, { "q", { .2, 8, ParameterScale::Logarithmic }, .707 } }, false, models::FilterScope::Detector } },
            { 9, { 9, FilterType::Gain, { { "gain", { -15, 15, ParameterScale::Linear }, 0 } }, false, models::FilterScope::Eq } }
        };
        return definitions;
    }

    static const std::map<long, models::FilterDefinition>& EqDefinitions() {
        static const auto definitions = FilterDefinitions(models::FilterScope::Eq);
        return definitions;
    }

    static const std::map<long, models::FilterDefinition>& DetectorDefinitions() {
        static const auto definitions = FilterDefinitions(models::FilterScope::Detector);
        return definitions;
    }

private:
    static std::map<long, models::FilterDefinition> FilterDefinitions(models::FilterScope scope) {
        std::map<long, models::FilterDefinition> result;
        for (const auto& [filterId, definition] : Definitions()) {
            if (definition.scope == scope) result.emplace(filterId, definition);
        }
        return result;
    }
};

} // namespace consolidator::settings
