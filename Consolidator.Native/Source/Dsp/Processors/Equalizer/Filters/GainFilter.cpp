#include "Dsp/Processors/Equalizer/Filters/GainFilter.h"

#include <cmath>

namespace consolidator::dsp
{

GainFilter::GainFilter(FilterId FilterId)
    : Filter(
          DeviceId::Equalizer,
          detail::ElementKind::EqFilter,
          detail::ToIndex(FilterId))
{
    RecalculateCoefficients();
}

void GainFilter::RecalculateCoefficients()
{
    BiquadCoefficients coefficients;
    coefficients.b0 = std::pow(10.0, runtimeState_.gainDb / 20.0);

    SetNormalizedCoefficients(coefficients);
}

} // namespace consolidator::dsp
