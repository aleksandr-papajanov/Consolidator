#pragma once

namespace consolidator::dsp
{

enum class BiquadType
{
    Bell,
    LowShelf,
    HighShelf,
    Gain
};

struct BiquadCoefficients
{
    double b0 = 1.0;
    double b1 = 0.0;
    double b2 = 0.0;
    double a1 = 0.0;
    double a2 = 0.0;
};

// Pure, shared biquad coefficient design used by DSP and analysis.
class BiquadDesigner final
{
  public:
    [[nodiscard]] static BiquadCoefficients Calculate(
        BiquadType type,
        double frequencyHz,
        double q,
        double gainDb,
        double sampleRate) noexcept;
};

} // namespace consolidator::dsp
