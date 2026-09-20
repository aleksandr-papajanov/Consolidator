#pragma once

#include <array>
#include <cstddef>

#include "DspParameterRamp.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

struct PolishAirTuning
{
    double baselineHighPassHz{ 5.0 };
    double directGain{ -0.068378072799221 };
    double lowerShelfFrequencyHz{ 6369.620913086973 };
    double lowerShelfGain{ -1.479709540746236 };
    double middleShelfFrequencyHz{ 10847.998233671118 };
    double middleShelfGain{ 2.983013520529492 };
    double upperShelfFrequencyHz{ 10877.071076642697 };
    double upperShelfGain{ 1.341741216533864 };
    double presencePeakFrequencyHz{ 4500.0 };
    double presencePeakQ{ 1.4 };
    double presencePeakGainDb{ 1.5 };
    double presenceNotchFrequencyHz{ 6400.0 };
    double presenceNotchQ{ 2.2 };
    double presenceNotchGainDb{ -0.5 };
    double levelShelfFrequencyHz{ 10000.0 };
    double levelShelfGain{ 3.0 };
    double levelBellFrequencyHz{ 10500.0 };
    double levelBellQ{ 1.3 };
    double levelBellGainDb{ -12.0 };
    double amountExponent{ 1.016154990256559 };
    double nonlinearSourceHighPassHz{ 1000.0 };
    double nonlinearOutputHighPassHz{ 13120.559086926609 };
    double oddHarmonicGain{ 0.038 };
    double fifthOrderGain{ 0.0072 };
    double seventhOrderGain{ 0.00078 };
    double evenHarmonicHighPassHz{ 8000.0 };
    double evenHarmonicGain{ 0.001 };
    std::array<double, 5> linearLevelGainAnchors{
        1.0,
        0.959463823225437,
        0.864072773551074,
        0.765270690864942,
        0.635640757649558
    };
};

class PolishAirProcessor final : public DspProcessor
{
public:
    void SetTuningBeforePrepare(const PolishAirTuning& tuning) noexcept;
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    struct BiquadCoefficients
    {
        double b0{ 1.0 };
        double b1{};
        double b2{};
        double a1{};
        double a2{};
    };

    struct BiquadState
    {
        double z1{};
        double z2{};
    };

    struct Channel
    {
        std::array<double, 1> baselineHighPass{};
        std::array<double, 1> lowerShelf{};
        std::array<double, 1> middleShelf{};
        std::array<double, 1> upperShelf{};
        std::array<double, 3> levelShelf{};
        std::array<double, 1> nonlinearSourceHighPass{};
        std::array<double, 2> nonlinearOutputHighPass{};
        std::array<double, 4> evenHarmonicHighPass{};
        BiquadState presencePeak{};
        BiquadState presenceNotch{};
        BiquadState levelBell{};
    };

    [[nodiscard]] static BiquadCoefficients CompileBell(
        double frequencyHz,
        double q,
        double gainDb,
        double sampleRate) noexcept;
    [[nodiscard]] static double ProcessBiquad(
        double input,
        const BiquadCoefficients& coefficients,
        BiquadState& state) noexcept;

    DspParameterRamp amount_;
    std::array<Channel, 2> channels_{};
    double baselineHighPassCoefficient_{};
    double lowerShelfCoefficient_{};
    double middleShelfCoefficient_{};
    double upperShelfCoefficient_{};
    double levelShelfCoefficient_{};
    double nonlinearSourceHighPassCoefficient_{};
    double nonlinearOutputHighPassCoefficient_{};
    double evenHarmonicHighPassCoefficient_{};
    BiquadCoefficients presencePeakCoefficients_{};
    BiquadCoefficients presenceNotchCoefficients_{};
    BiquadCoefficients levelBellCoefficients_{};
    double peakReleaseCoefficient_{};
    std::size_t peakHoldSamples_{};
    std::size_t peakHoldRemaining_{};
    double inputPeak_{};
    double targetAmountGain_{};
    PolishAirTuning tuning_{};
};

} // namespace consolidator::max::dsp
