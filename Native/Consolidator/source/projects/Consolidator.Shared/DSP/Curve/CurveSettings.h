#pragma once

#include "../../Settings/GlobalSettings.h"

#include <cstddef>

namespace consolidator::dsp {

enum class CurveScale {
    Linear,
    Logarithmic
};

struct CurveSettings {
    std::size_t pointCount = settings::GlobalSettings::DefaultCurvePointCount;
    double minimumInput = settings::GlobalSettings::MinimumFrequencyHz;
    double maximumInput = settings::GlobalSettings::MaximumFrequencyHz;
    CurveScale scale = CurveScale::Logarithmic;

    bool operator==(const CurveSettings&) const = default;
};

} // namespace consolidator::dsp
