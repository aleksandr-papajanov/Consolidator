#include "KWeighting.h"

#include <cmath>
#include <numbers>

#include "../Math.h"

namespace consolidator::audio::primitives::filters
{

void KWeighting::Prepare(double sampleRate) noexcept
{
    // Continuous parameters reproduce the BS.1770 48 kHz reference response;
    // the bilinear transform prepares coefficients at the actual sample rate.
    const auto shelfTangent = std::tan(std::numbers::pi * 1681.974450955533 / sampleRate);
    constexpr double kShelfQ = 0.7071752369554196;
    const auto highGain = DbToGain(3.999843853973347);
    const auto bandGain = std::pow(highGain, 0.4996667741545416);
    const auto shelfSquare = shelfTangent * shelfTangent;
    const auto shelfDenominator = 1.0 + shelfTangent / kShelfQ + shelfSquare;
    shelf_.coefficients = {
        (highGain + bandGain * shelfTangent / kShelfQ + shelfSquare) / shelfDenominator,
        2.0 * (shelfSquare - highGain) / shelfDenominator,
        (highGain - bandGain * shelfTangent / kShelfQ + shelfSquare) / shelfDenominator,
        2.0 * (shelfSquare - 1.0) / shelfDenominator,
        (1.0 - shelfTangent / kShelfQ + shelfSquare) / shelfDenominator
    };
    const auto highPassTangent = std::tan(std::numbers::pi * 38.13547087602444 / sampleRate);
    constexpr double kHighPassQ = 0.5003270373238773;
    const auto highPassSquare = highPassTangent * highPassTangent;
    const auto highPassDenominator = 1.0 + highPassTangent / kHighPassQ + highPassSquare;
    // RLB weighting retains the reference numerator; this is not a unity-normalized audio HP.
    highPass_.coefficients = {
        1.0, -2.0, 1.0,
        2.0 * (highPassSquare - 1.0) / highPassDenominator,
        (1.0 - highPassTangent / kHighPassQ + highPassSquare) / highPassDenominator
    };
    Reset();
}

void KWeighting::Reset() noexcept
{
    shelf_.Reset();
    highPass_.Reset();
}

} // namespace consolidator::audio::primitives::filters
