#include "Dsp/Processors/Equalizer/Filters/HighShelfFilter.h"

#include <cmath>
#include <numbers>

namespace consolidator::dsp
{

HighShelfFilter::HighShelfFilter(
    EqFilterId filterId,
    double frequencyHz)
    : Filter(
          DeviceId::Equalizer,
          detail::ElementKind::EqFilter,
          detail::ToIndex(filterId))
{
    InitializeParameters(
        frequencyHz,
        core::settings::FilterDefaults::kDefaultQ,
        core::settings::FilterDefaults::kDefaultGainDb);

    RecalculateCoefficients();
    SyncState();
}

void HighShelfFilter::RecalculateCoefficients()
{
    const double gain =
        std::pow(10.0, parameters_.gainDb / 40.0);

    const double omega =
        2.0 * std::numbers::pi * parameters_.frequencyHz / sampleRate_;

    const double sine = std::sin(omega);
    const double cosine = std::cos(omega);
    const double alpha = sine / (2.0 * parameters_.q);
    const double gainRootTerm =
        2.0 * std::sqrt(gain) * alpha;

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
    SyncState();
}

void HighShelfFilter::SyncState()
{
    state_.frequency =
        static_cast<float>(parameters_.frequencyHz);

    state_.q =
        static_cast<float>(parameters_.q);

    state_.gainDb =
        static_cast<float>(parameters_.gainDb);

    state_.bypass = parameters_.bypass;
}

} // namespace consolidator::dsp