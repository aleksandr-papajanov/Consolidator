#include "Dsp/Processors/Equalizer/Filters/BiquadDesigner.h"

#include <cmath>
#include <numbers>

namespace consolidator::dsp
{

BiquadCoefficients BiquadDesigner::Calculate(
    BiquadType type,
    double frequencyHz,
    double q,
    double gainDb,
    double sampleRate) noexcept
{
    if (type == BiquadType::Gain)
    {
        return {std::pow(10.0, gainDb / 20.0), 0.0, 0.0, 0.0, 0.0};
    }

    const auto omega = 2.0 * std::numbers::pi * frequencyHz / sampleRate;
    const auto sine = std::sin(omega);
    const auto cosine = std::cos(omega);
    const auto alpha = sine / (2.0 * q);
    const auto amplitude = std::pow(10.0, gainDb / 40.0);
    if (type == BiquadType::Bell)
    {
        const auto inverseA0 = 1.0 / (1.0 + alpha / amplitude);
        return {(1.0 + alpha * amplitude) * inverseA0,
                -2.0 * cosine * inverseA0,
                (1.0 - alpha * amplitude) * inverseA0,
                -2.0 * cosine * inverseA0,
                (1.0 - alpha / amplitude) * inverseA0};
    }

    const auto gain = amplitude;
    const auto gainRootTerm = 2.0 * std::sqrt(gain) * alpha;
    if (type == BiquadType::LowShelf)
    {
        const auto inverseA0 = 1.0 / ((gain + 1.0) + (gain - 1.0) * cosine + gainRootTerm);
        return {gain * ((gain + 1.0) - (gain - 1.0) * cosine + gainRootTerm) * inverseA0,
                2.0 * gain * ((gain - 1.0) - (gain + 1.0) * cosine) * inverseA0,
                gain * ((gain + 1.0) - (gain - 1.0) * cosine - gainRootTerm) * inverseA0,
                -2.0 * ((gain - 1.0) + (gain + 1.0) * cosine) * inverseA0,
                ((gain + 1.0) + (gain - 1.0) * cosine - gainRootTerm) * inverseA0};
    }

    const auto inverseA0 = 1.0 / ((gain + 1.0) - (gain - 1.0) * cosine + gainRootTerm);
    return {gain * ((gain + 1.0) + (gain - 1.0) * cosine + gainRootTerm) * inverseA0,
            -2.0 * gain * ((gain - 1.0) + (gain + 1.0) * cosine) * inverseA0,
            gain * ((gain + 1.0) + (gain - 1.0) * cosine - gainRootTerm) * inverseA0,
            2.0 * ((gain - 1.0) - (gain + 1.0) * cosine) * inverseA0,
            ((gain + 1.0) - (gain - 1.0) * cosine - gainRootTerm) * inverseA0};
}

} // namespace consolidator::dsp
