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

BellFilter::BellFilter(FilterId FilterId, double frequencyHz)
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

void BellFilter::RecalculateCoefficients()
{
    const double amplitude = GainDbToAmplitude(runtimeState_.gainDb);
    const double omega = 2.0 * std::numbers::pi * runtimeState_.frequencyHz / GetSampleRate();
    const double alpha = std::sin(omega) / (2.0 * runtimeState_.q);

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
}

} // namespace consolidator::dsp
