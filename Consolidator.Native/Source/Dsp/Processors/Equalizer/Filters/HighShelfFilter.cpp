#include "Dsp/Processors/Equalizer/Filters/HighShelfFilter.h"

#include <cmath>
#include <numbers>

namespace consolidator::dsp
{

HighShelfFilter::HighShelfFilter(
    FilterId FilterId,
    double frequencyHz)
    : Filter(
          DeviceId::Equalizer,
          detail::ElementKind::EqFilter,
          detail::ToIndex(FilterId))
{
    InitializeParameters(
        frequencyHz,
        core::settings::FilterDefaults::kDefaultQ,
        core::settings::FilterDefaults::kDefaultGainDb);

    RecalculateCoefficients();
}

void HighShelfFilter::RecalculateCoefficients()
{
    const double gain = std::pow(10.0, runtimeState_.gainDb / 40.0);

    const double omega = 2.0 * std::numbers::pi * runtimeState_.frequencyHz / GetSampleRate();

    const double sine = std::sin(omega);
    const double cosine = std::cos(omega);
    const double alpha = sine / (2.0 * runtimeState_.q);
    const double gainRootTerm = 2.0 * std::sqrt(gain) * alpha;

    const double a0 =
        (gain + 1.0) -
        (gain - 1.0) * cosine +
        gainRootTerm;

    const double inverseA0 = 1.0 / a0;

    BiquadCoefficients coefficients;

    coefficients.b0 =
        gain *
        ((gain + 1.0) +
         (gain - 1.0) * cosine +
         gainRootTerm) *
        inverseA0;

    coefficients.b1 =
        -2.0 *
        gain *
        ((gain - 1.0) +
         (gain + 1.0) * cosine) *
        inverseA0;

    coefficients.b2 =
        gain *
        ((gain + 1.0) +
         (gain - 1.0) * cosine -
         gainRootTerm) *
        inverseA0;

    coefficients.a1 =
        2.0 *
        ((gain - 1.0) -
         (gain + 1.0) * cosine) *
        inverseA0;

    coefficients.a2 =
        ((gain + 1.0) -
         (gain - 1.0) * cosine -
         gainRootTerm) *
        inverseA0;

    SetNormalizedCoefficients(coefficients);
}

} // namespace consolidator::dsp
