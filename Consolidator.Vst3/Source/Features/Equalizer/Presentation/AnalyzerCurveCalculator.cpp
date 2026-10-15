#include "Features/Equalizer/Presentation/AnalyzerCurveCalculator.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <numbers>

namespace consolidator::vst3::features::equalizer::presentation
{
namespace
{

struct Coefficients
{
    double b0{ 1 }, b1{}, b2{}, a1{}, a2{};
};

Coefficients Shelf(bool low, double frequency, double q, double gain, double sampleRate)
{
    const double omega = 2 * std::numbers::pi * frequency / sampleRate;
    const double cosine = std::cos(omega);
    const double amplitude = std::pow(10.0, gain / 40);
    const double beta = std::sqrt(amplitude) * std::sin(omega) / q;
    const double a0 = low
        ? amplitude + 1 + (amplitude - 1) * cosine + beta
        : amplitude + 1 - (amplitude - 1) * cosine + beta;
    return {
        amplitude * (amplitude + 1 + (low ? -1 : 1) * (amplitude - 1) * cosine + beta) / a0,
        (low ? 2 : -2) * amplitude * (amplitude - 1 + (low ? -1 : 1) * (amplitude + 1) * cosine) / a0,
        amplitude * (amplitude + 1 + (low ? -1 : 1) * (amplitude - 1) * cosine - beta) / a0,
        (low ? -2 : 2) * (amplitude - 1 + (low ? 1 : -1) * (amplitude + 1) * cosine) / a0,
        (amplitude + 1 + (low ? 1 : -1) * (amplitude - 1) * cosine - beta) / a0
    };
}

Coefficients Bell(double frequency, double q, double gain, double sampleRate)
{
    const double omega = 2 * std::numbers::pi * frequency / sampleRate;
    const double alpha = std::sin(omega) / (2 * q);
    const double amplitude = std::pow(10.0, gain / 40);
    const double inverseA0 = 1 / (1 + alpha / amplitude);
    const double cosine = std::cos(omega);
    return {
        (1 + alpha * amplitude) * inverseA0,
        -2 * cosine * inverseA0,
        (1 - alpha * amplitude) * inverseA0,
        -2 * cosine * inverseA0,
        (1 - alpha / amplitude) * inverseA0
    };
}

struct ResponseBasis
{
    double cosine;
    double sine;
    double cosine2;
    double sine2;
};

double DecibelsAt(const Coefficients& coefficients, const ResponseBasis& basis)
{
    const double numerator = std::hypot(
        coefficients.b0 + coefficients.b1 * basis.cosine + coefficients.b2 * basis.cosine2,
        -coefficients.b1 * basis.sine - coefficients.b2 * basis.sine2);
    const double denominator = std::hypot(
        1 + coefficients.a1 * basis.cosine + coefficients.a2 * basis.cosine2,
        -coefficients.a1 * basis.sine - coefficients.a2 * basis.sine2);
    return 20 * std::log10(std::max(numerator / std::max(denominator, 1e-12), 1e-12));
}

} // namespace

double AnalyzerCurveCalculator::FrequencyAt(std::size_t point)
{
    static const auto frequencies = []
    {
        std::array<double, AnalyzerCurves::kPointCount> result{};
        for (std::size_t index = 0; index < result.size(); ++index)
        {
            result[index] = 20 * std::pow(1000.0,
                static_cast<double>(index) / static_cast<double>(result.size() - 1));
        }
        return result;
    }();
    return frequencies[point];
}

AnalyzerCurves AnalyzerCurveCalculator::Calculate(
    std::span<const AnalyzerFilter> filters, double sampleRate, bool bankActive)
{
    AnalyzerCurves result;
    result.filters.resize(filters.size());
    result.combined.fill(0.5);
    std::array<ResponseBasis, AnalyzerCurves::kPointCount> basis;
    for (std::size_t point = 0; point < basis.size(); ++point)
    {
        const double omega = 2 * std::numbers::pi * FrequencyAt(point) / sampleRate;
        basis[point] = { std::cos(omega), std::sin(omega), std::cos(2 * omega), std::sin(2 * omega) };
    }
    for (std::size_t index = 0; index < filters.size(); ++index)
    {
        const auto& filter = filters[index];
        const double frequency = std::min(filter.frequency, sampleRate * 0.499);
        Coefficients first, second;
        if (filter.gain != 0 && filter.type != AnalyzerFilterType::Gain)
        {
            if (filter.type == AnalyzerFilterType::Bell)
            {
                first = Bell(frequency, filter.q, filter.gain, sampleRate);
            }
            else
            {
                const bool tilt = filter.type == AnalyzerFilterType::Tilt;
                first = Shelf(tilt || filter.type == AnalyzerFilterType::LowShelf,
                    frequency, filter.q, tilt ? -filter.gain : filter.gain, sampleRate);
                if (tilt)
                {
                    second = Shelf(false, frequency, filter.q, filter.gain, sampleRate);
                }
            }
        }
        for (std::size_t point = 0; point < AnalyzerCurves::kPointCount; ++point)
        {
            double decibels = filter.type == AnalyzerFilterType::Gain
                ? filter.gain : DecibelsAt(first, basis[point]);
            if (filter.type == AnalyzerFilterType::Tilt)
            {
                decibels += DecibelsAt(second, basis[point]);
            }
            result.filters[index][point] = 0.5 - decibels / 48;
            if (bankActive && filter.active)
            {
                result.combined[point] -= decibels / 48;
            }
        }
    }
    return result;
}

} // namespace consolidator::vst3::features::equalizer::presentation
