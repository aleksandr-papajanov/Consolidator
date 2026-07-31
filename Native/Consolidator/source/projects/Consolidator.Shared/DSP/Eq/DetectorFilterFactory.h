#pragma once

#include "Filters/FilterSettings.h"
#include "../../Models/DetectorFilterState.h"
#include "../../Settings/FilterTopology.h"
#include <cmath>
#include <string>

namespace consolidator::dsp {

class DetectorFilterFactory final {
public:
    static BellFilterSettings Settings(
        const models::DetectorFilterState& state,
        double sampleRate
    ) {
        const auto& definition = settings::FilterTopology::DetectorDefinition(state.filterId);
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
        return std::isfinite(value) ? value : parameter->defaultValue;
    }
};

} // namespace consolidator::dsp
