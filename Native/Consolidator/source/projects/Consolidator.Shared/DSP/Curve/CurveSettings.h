#pragma once

#include "../../Settings/AnalysisOptions.h"
#include "../../Settings/SpectrumOptions.h"

#include <cstddef>

namespace consolidator::dsp {

enum class CurveScale {
    Linear,
    Logarithmic
};

struct CurveSettings {
    std::size_t pointCount = settings::AnalysisOptions::DefaultCurvePointCount;
    double minimumInput = settings::SpectrumOptions::MinimumFrequencyHz;
    double maximumInput = settings::SpectrumOptions::MaximumFrequencyHz;
    CurveScale scale = CurveScale::Logarithmic;

    bool operator==(const CurveSettings&) const = default;
};

} // namespace consolidator::dsp
