#include "Dsp/Processors/Equalizer/Filters/GainFilter.h"

#include <cmath>

namespace consolidator::dsp
{

GainFilter::GainFilter(EqFilterId filterId)
    : Filter(
          DeviceId::Equalizer,
          detail::ElementKind::EqFilter,
          detail::ToIndex(filterId))
{
    RecalculateCoefficients();
    SyncState();
}

void GainFilter::RecalculateCoefficients()
{
    coefficients_.b0 =
        std::pow(10.0, parameters_.gainDb / 20.0);

    coefficients_.b1 = 0.0;
    coefficients_.b2 = 0.0;
    coefficients_.a1 = 0.0;
    coefficients_.a2 = 0.0;

    SyncState();
}

void GainFilter::SyncState()
{
    state_.gainDb =
        static_cast<float>(parameters_.gainDb);

    state_.bypass = parameters_.bypass;
}

} // namespace consolidator::dsp