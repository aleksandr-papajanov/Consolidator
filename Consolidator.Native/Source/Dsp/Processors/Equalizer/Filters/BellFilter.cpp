#include "Dsp/Processors/Equalizer/Filters/BellFilter.h"

#include <cmath>
#include <numbers>

namespace consolidator::dsp
{

namespace
{

double GainDbToAmplitude(double gainDb)
{
    return std::pow(10.0, gainDb / 40.0);
}

} // namespace

BellFilter::BellFilter(EqFilterId filterId, double frequencyHz)
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

void BellFilter::RecalculateCoefficients()
{
    const double amplitude = GainDbToAmplitude(parameters_.gainDb);
    const double omega =
        2.0 * std::numbers::pi * parameters_.frequencyHz / sampleRate_;
    const double alpha =
        std::sin(omega) / (2.0 * parameters_.q);

    const double a0 = 1.0 + alpha / amplitude;
    const double inverseA0 = 1.0 / a0;
    const double cosine = std::cos(omega);

    BiquadCoefficients coefficients;
    coefficients.b0 = (1.0 + alpha * amplitude) * inverseA0;
    coefficients.b1 = (-2.0 * cosine) * inverseA0;
    coefficients.b2 = (1.0 - alpha * amplitude) * inverseA0;
    coefficients.a1 = (-2.0 * cosine) * inverseA0;
    coefficients.a2 = (1.0 - alpha / amplitude) * inverseA0;

    SetNormalizedCoefficients(coefficients);
    SyncState();
}

void BellFilter::SyncState()
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