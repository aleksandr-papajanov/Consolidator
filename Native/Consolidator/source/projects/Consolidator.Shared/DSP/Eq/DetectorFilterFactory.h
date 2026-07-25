#pragma once

#include "Filters/FilterSettings.h"
#include "../../Models/DetectorFilterState.h"
#include "../../Settings/DetectorFilterOptions.h"
#include "../../Helpers/NumericHelper.h"

#include <string>

namespace consolidator::dsp {

class DetectorFilterFactory final {
public:
    static BellFilterSettings Settings(
        const models::DetectorFilterState& state,
        double sampleRate
    ) {
        const auto& definition = settings::DetectorFilterOptions::Definition();
        return {
            Value(state.frequencyHz, definition, "frequency"),
            Value(state.q, definition, "q"),
            Value(state.gainDb, definition, "gain"),
            sampleRate
        };
    }

private:
    static double Value(
        double value,
        const models::FilterDefinition& definition,
        const std::string& name
    ) {
        const auto* parameter = definition.FindParameter(name);
        if (!parameter) return 0.0;
        return helpers::NumericHelper::ClampFinite(
            value,
            parameter->range.minimum,
            parameter->range.maximum,
            parameter->defaultValue
        );
    }
};

} // namespace consolidator::dsp
