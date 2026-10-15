#include "Biquad.h"

#include <cmath>
#include <numbers>

namespace consolidator::audio::primitives::filters
{

BiquadCoefficients DesignHighPass(
    double frequencyHz,
    double q,
    double sampleRate) noexcept
{
    const auto omega = 2.0 * std::numbers::pi * frequencyHz / sampleRate;
    const auto sine = std::sin(omega);
    const auto cosine = std::cos(omega);
    const auto alpha = sine / (2.0 * q);
    const auto denominator = 1.0 + alpha;
    const auto numerator = (1.0 + cosine) / (2.0 * denominator);
    return {
        numerator,
        -2.0 * numerator,
        numerator,
        -2.0 * cosine / denominator,
        (1.0 - alpha) / denominator
    };
}

BiquadCoefficients DesignBell(
    double frequencyHz,
    double q,
    double gainDb,
    double sampleRate) noexcept
{
    const auto omega = 2.0 * std::numbers::pi * frequencyHz / sampleRate;
    const auto sine = std::sin(omega);
    const auto cosine = std::cos(omega);
    const auto alpha = sine / (2.0 * q);
    const auto amplitude = std::pow(10.0, gainDb / 40.0);
    const auto inverseA0 = 1.0 / (1.0 + alpha / amplitude);
    return {
        (1.0 + alpha * amplitude) * inverseA0,
        -2.0 * cosine * inverseA0,
        (1.0 - alpha * amplitude) * inverseA0,
        -2.0 * cosine * inverseA0,
        (1.0 - alpha / amplitude) * inverseA0
    };
}

BiquadCoefficients DesignShelf(
    bool lowShelf,
    double frequencyHz,
    double q,
    double gainDb,
    double sampleRate) noexcept
{
    const auto omega = 2.0 * std::numbers::pi * frequencyHz / sampleRate;
    const auto sine = std::sin(omega);
    const auto cosine = std::cos(omega);
    const auto amplitude = std::pow(10.0, gainDb / 40.0);
    const auto alpha = sine / (2.0 * q);
    const auto beta = 2.0 * std::sqrt(amplitude) * alpha;
    const auto b0 = lowShelf
        ? amplitude * ((amplitude + 1.0) -
            (amplitude - 1.0) * cosine + beta)
        : amplitude * ((amplitude + 1.0) +
            (amplitude - 1.0) * cosine + beta);
    const auto b1 = lowShelf
        ? 2.0 * amplitude * ((amplitude - 1.0) -
            (amplitude + 1.0) * cosine)
        : -2.0 * amplitude * ((amplitude - 1.0) +
            (amplitude + 1.0) * cosine);
    const auto b2 = lowShelf
        ? amplitude * ((amplitude + 1.0) -
            (amplitude - 1.0) * cosine - beta)
        : amplitude * ((amplitude + 1.0) +
            (amplitude - 1.0) * cosine - beta);
    const auto a0 = lowShelf
        ? (amplitude + 1.0) + (amplitude - 1.0) * cosine + beta
        : (amplitude + 1.0) - (amplitude - 1.0) * cosine + beta;
    const auto a1 = lowShelf
        ? -2.0 * ((amplitude - 1.0) +
            (amplitude + 1.0) * cosine)
        : 2.0 * ((amplitude - 1.0) -
            (amplitude + 1.0) * cosine);
    const auto a2 = lowShelf
        ? (amplitude + 1.0) + (amplitude - 1.0) * cosine - beta
        : (amplitude + 1.0) - (amplitude - 1.0) * cosine - beta;
    const auto inverseA0 = 1.0 / a0;
    return {
        b0 * inverseA0,
        b1 * inverseA0,
        b2 * inverseA0,
        a1 * inverseA0,
        a2 * inverseA0
    };
}

} // namespace consolidator::audio::primitives::filters
