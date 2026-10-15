#pragma once

#include <array>
#include <cstddef>
#include <span>
#include <vector>

namespace consolidator::vst3::features::equalizer::presentation
{

enum class AnalyzerFilterType
{
    Gain,
    Tilt,
    LowShelf,
    HighShelf,
    Bell
};

// Presentation input supplied from authoritative bindings, never DSP state.
struct AnalyzerFilter
{
    AnalyzerFilterType type;
    double frequency;
    double q;
    double gain;
    bool active;
};

struct AnalyzerCurves
{
    static constexpr std::size_t kPointCount = 1024;
    using Curve = std::array<double, kPointCount>;
    std::vector<Curve> filters;
    Curve combined{};
};

class AnalyzerCurveCalculator final
{
public:
    // Normalized plot coordinates: 0 = +24 dB, 1 = -24 dB.
    // Values outside the visible range remain unclamped for renderer clipping.
    // Caller supplies a prepared positive sample rate and valid finite bindings
    // (positive frequency and Q); this class does not hydrate missing state.
    [[nodiscard]] static AnalyzerCurves Calculate(
        std::span<const AnalyzerFilter> filters, double sampleRate, bool bankActive);
    [[nodiscard]] static double FrequencyAt(std::size_t point);
};

} // namespace consolidator::vst3::features::equalizer::presentation
