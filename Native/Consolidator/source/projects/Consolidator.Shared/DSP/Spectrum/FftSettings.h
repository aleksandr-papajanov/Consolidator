#pragma once

#include "../../Helpers/NumericHelper.h"
#include "../../Settings/AnalysisOptions.h"

#include <cstddef>

namespace consolidator::dsp {

struct FftSettings {
    std::size_t size = settings::AnalysisOptions::DefaultFftSize;

    bool IsValid() const {
        return helpers::NumericHelper::IsPowerOfTwo(size);
    }

    bool operator==(const FftSettings&) const = default;
};

} // namespace consolidator::dsp
