#pragma once

namespace consolidator::max::dsp::processors
{

// Selected at construction, never changed by audio automation or current VST3 adapters.
enum class CompressorControlMode
{
    Normal,
    Emulation
};

struct CompressorControls
{
    double reduction{ 0.5 };
    double outputGain{ 1.0 };
};

// A sensitivity range, not a promise of this much gain reduction on every signal.
inline constexpr double kNormalThresholdMaximumDb = 12.0;
inline constexpr double kNormalThresholdSpanDb = 60.0;
inline constexpr double kNormalRatioDefault = 0.4;
inline constexpr double kNormalRatioFourToOnePosition = 3.0 / 99.0;
inline constexpr double kCompressorOutputMinimumDb = -24.0;
inline constexpr double kCompressorOutputMaximumDb = 24.0;

// Map knob position to the internal linear 1..100 coordinate: 0 -> 1:1, 0.4 -> 4:1, 1 -> 100.
// A rational taper gives useful travel to low ratios without a lookup table or power function.
inline double NormalRatioPosition(double control) noexcept
{
    return 3.0 * control / (64.0 - 61.0 * control);
}

} // namespace consolidator::max::dsp::processors
