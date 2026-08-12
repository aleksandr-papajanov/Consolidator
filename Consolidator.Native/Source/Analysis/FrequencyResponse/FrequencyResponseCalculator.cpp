#include "Analysis/FrequencyResponse/FrequencyResponseCalculator.h"

#include <algorithm>
#include <cmath>
#include <complex>
#include <numbers>

namespace consolidator::analysis
{

namespace
{

constexpr double kMinimumFrequencyHz = 20.0;
constexpr double kMaximumFrequencyHz = 20000.0;
constexpr double kMinimumMagnitudeDb = -240.0;
constexpr double kMinimumDenominator = 1.0e-24;

double CalculateMagnitude(
    const FrequencyResponseRequest& request,
    double angularFrequency)
{
    const std::complex<double> z{std::cos(angularFrequency),
                                 std::sin(angularFrequency)};
    const auto zSquared = z * z;
    std::complex<double> response{1.0, 0.0};
    const auto stageCount = std::min(request.stageCount, request.stages.size());

    for (std::size_t index = 0; index < stageCount; ++index)
    {
        const auto& stage = request.stages[index];
        const auto numerator = stage.b0 + stage.b1 / z + stage.b2 / zSquared;
        const auto denominator = 1.0 + stage.a1 / z + stage.a2 / zSquared;
        if (std::norm(denominator) < kMinimumDenominator)
        {
            return 0.0;
        }
        response *= numerator / denominator;
    }

    return std::abs(response);
}

} // namespace

void FrequencyResponseCalculator::Calculate(
    const FrequencyResponseRequest& request,
    FrequencyResponseSnapshot& output) const noexcept
{
    output.sourceRevision = request.revision;
    if (request.sampleRate <= 0.0)
    {
        output.magnitudeDb.fill(static_cast<float>(kMinimumMagnitudeDb));
        return;
    }

    const auto maximumFrequency = std::min(
        kMaximumFrequencyHz,
        request.sampleRate * 0.5);
    if (maximumFrequency <= kMinimumFrequencyHz)
    {
        output.magnitudeDb.fill(static_cast<float>(kMinimumMagnitudeDb));
        return;
    }

    const auto logMinimum = std::log(kMinimumFrequencyHz);
    const auto logMaximum = std::log(maximumFrequency);
    for (std::size_t index = 0; index < kResponsePointCount; ++index)
    {
        const auto position = static_cast<double>(index) / static_cast<double>(kResponsePointCount - 1);
        const auto frequency = std::exp(
            logMinimum + position * (logMaximum - logMinimum));
        const auto angularFrequency =
            2.0 * std::numbers::pi * frequency / request.sampleRate;
        const auto magnitude = CalculateMagnitude(request, angularFrequency);
        const auto magnitudeDb = magnitude > 0.0
                                     ? 20.0 * std::log10(magnitude)
                                     : kMinimumMagnitudeDb;
        output.magnitudeDb[index] = static_cast<float>(std::max(
            magnitudeDb,
            kMinimumMagnitudeDb));
    }
}

} // namespace consolidator::analysis
