#pragma once

#include "../../Helpers/NumericHelper.h"
#include "../../Settings/GlobalSettings.h"

#include <cstddef>

namespace consolidator::dsp {

struct FftSettings {
    std::size_t size = settings::GlobalSettings::DefaultFftSize;

    bool IsValid() const {
        return helpers::NumericHelper::IsPowerOfTwo(size);
    }

    bool operator==(const FftSettings&) const = default;
};

} // namespace consolidator::dsp
