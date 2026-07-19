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
            { 1, { 1, FilterType::Gain, { { "gain", { -15, 15, ParameterScale::Linear }, 0 } }, false } },
            { 2, { 2, FilterType::Tilt, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "pivot", { 40, 18000, ParameterScale::Logarithmic }, 1000 } }, false } },
            { 3, { 3, FilterType::LowShelf, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "freq", { 40, 450, ParameterScale::Logarithmic }, 200 }, { "q", { .2, 1, ParameterScale::Logarithmic }, .707 } }, false } },
            { 4, { 4, FilterType::Peak, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "freq", { 200, 2500, ParameterScale::Logarithmic }, 1000 }, { "q", { .2, 1, ParameterScale::Logarithmic }, 1 } }, false } },
            { 5, { 5, FilterType::Peak, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "freq", { 600, 7000, ParameterScale::Logarithmic }, 3000 }, { "q", { .2, 1, ParameterScale::Logarithmic }, 1 } }, false } },
            { 6, { 6, FilterType::HighShelf, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "freq", { 1500, 18000, ParameterScale::Logarithmic }, 8000 }, { "q", { .2, 1, ParameterScale::Logarithmic }, .707 } }, false } },
            { 7, { 7, FilterType::Peak, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "freq", { 40, 600, ParameterScale::Logarithmic }, 200 }, { "q", { 1, 7, ParameterScale::Logarithmic }, 3 } }, false } },
            { 8, { 8, FilterType::Peak, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "freq", { 600, 3000, ParameterScale::Logarithmic }, 1000 }, { "q", { 1, 7, ParameterScale::Logarithmic }, 3 } }, false } },
            { 9, { 9, FilterType::Peak, { { "gain", { -15, 15, ParameterScale::Linear }, 0 }, { "freq", { 3000, 18000, ParameterScale::Logarithmic }, 5000 }, { "q", { 1, 7, ParameterScale::Logarithmic }, 3 } }, false } }
        };
        return definitions;
    }
};

} // namespace consolidator::settings
