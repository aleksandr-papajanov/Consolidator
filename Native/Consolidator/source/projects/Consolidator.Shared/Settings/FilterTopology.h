#pragma once

#include "../Models/FilterDefinition.h"

#include <map>
#include <string_view>

namespace consolidator::settings {

class FilterTopology final {
public:
    static const std::map<long, models::FilterDefinition>& Definitions() {
        using models::FilterType;
        static const std::map<long, models::FilterDefinition> definitions = {
            { 1, { 1, FilterType::Tilt, { { "gain", {}, 0 }, { "pivot", {}, 1000 } }, false } },
            { 2, { 2, FilterType::LowShelf, { { "gain", {}, 0 }, { "freq", {}, 200 } }, false } },
            { 3, { 3, FilterType::HighShelf, { { "gain", {}, 0 }, { "freq", {}, 8000 } }, false } },
            { 4, { 4, FilterType::Peak, { { "gain", {}, 0 }, { "freq", {}, 1000 }, { "q", {}, 1 } }, false } },
            { 5, { 5, FilterType::Peak, { { "gain", {}, 0 }, { "freq", {}, 3000 }, { "q", {}, 1 } }, false } },
            { 6, { 6, FilterType::Peak, { { "gain", {}, 0 }, { "freq", {}, 500 }, { "q", {}, 3 } }, false, models::FilterScope::Eq } },
            { 7, { 7, FilterType::Peak, { { "gain", {}, 0 }, { "frequency", {}, 200 }, { "q", {}, .707 } }, false, models::FilterScope::Detector } },
            { 8, { 8, FilterType::Peak, { { "gain", {}, 0 }, { "frequency", {}, 4000 }, { "q", {}, .707 } }, false, models::FilterScope::Detector } },
            { 9, { 9, FilterType::Gain, { { "gain", {}, 0 } }, false, models::FilterScope::Eq } }
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

    static const models::FilterDefinition& DetectorDefinition(long filterId) {
        return Definitions().at(filterId + 6);
    }

    static double DetectorDefaultValue(long filterId, std::string_view name) {
        for (const auto& parameter : DetectorDefinition(filterId).parameters) {
            if (parameter.name == name) return parameter.defaultValue;
        }
        return 0.0;
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
