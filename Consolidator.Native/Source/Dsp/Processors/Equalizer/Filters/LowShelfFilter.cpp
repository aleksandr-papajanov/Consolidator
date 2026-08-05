#include "Dsp/Processors/Equalizer/Filters/LowShelfFilter.h"
#include <cmath>
#include <numbers>
namespace consolidator::dsp
{
LowShelfFilter::LowShelfFilter(EqFilterId filterId, double frequencyHz)
    : Filter(DeviceId::Equalizer, detail::ElementKind::EqFilter, detail::ToIndex(filterId))
{ InitializeParameters(frequencyHz, core::settings::FilterDefaults::kDefaultQ, core::settings::FilterDefaults::kDefaultGainDb); RecalculateCoefficients(); SyncState(); }
void LowShelfFilter::RecalculateCoefficients()
{
    const double gain = std::pow(10.0, parameters_.gainDb / 40.0);
    const double omega = 2.0 * std::numbers::pi * parameters_.frequencyHz / sampleRate_;
    const double alpha = std::sin(omega) / (2.0 * parameters_.q);
    const double cosO = std::cos(omega), sqrt2A = 2.0 * std::sqrt(gain) * alpha;
    const double a0 = (gain + 1.0) + (gain - 1.0) * cosO + sqrt2A, invA0 = 1.0 / a0;
    BiquadCoefficients c;
    c.b0 = gain * ((gain + 1.0) - (gain - 1.0) * cosO + sqrt2A) * invA0;
    c.b1 = 2.0 * gain * ((gain - 1.0) - (gain + 1.0) * cosO) * invA0;
    c.b2 = gain * ((gain + 1.0) - (gain - 1.0) * cosO - sqrt2A) * invA0;
    c.a1 = -2.0 * ((gain - 1.0) + (gain + 1.0) * cosO) * invA0;
    c.a2 = ((gain + 1.0) + (gain - 1.0) * cosO - sqrt2A) * invA0;
    SetNormalizedCoefficients(c); SyncState();
}
void LowShelfFilter::SyncState() { state_.frequency = static_cast<float>(parameters_.frequencyHz); state_.q = static_cast<float>(parameters_.q); state_.gainDb = static_cast<float>(parameters_.gainDb); state_.bypass = parameters_.bypass; }
}