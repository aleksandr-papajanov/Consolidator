#pragma once

#include <array>
#include <cstddef>

#include "../../Primitives/ParameterRamp.h"
#include "../../Primitives/Filters/Biquad.h"
#include "../../Core/AudioBuffer.h"

namespace consolidator::audio::algorithms
{

struct PolishAirConfig
{
    double baselineHighPassHz{ 5.0 };
    double directGain{ -0.0684 };
    double lowerShelfFrequencyHz{ 6370.0 };
    double lowerShelfGain{ -1.48 };
    double upperShelfFrequencyHz{ 10860.0 };
    double upperShelfGain{ 4.325 };
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
    double nonlinearSourceHighPassHz{ 1000.0 };
    double nonlinearOutputHighPassHz{ 13120.0 };
    double oddHarmonicGain{ 0.038 };
    // 1 + u*(c0 + c1*u), u=clamp((input dB + 30)/30, 0, 1).
    std::array<double, 2> linearLevelLaw{ 0.0086, -0.3792 };
};

// Additive contour and cubic harmonics; only the contour follows the linked held peak.
class PolishAirProcessor final
{
public:
    void Prepare(const ProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetAmount(double value) noexcept;
    void Process(AudioBuffer buffer, ChannelMode mode) noexcept;

private:
    struct Channel
    {
        std::array<double, 1> baselineHighPass{};
        std::array<double, 1> lowerShelf{};
        std::array<double, 1> upperShelf{};
        std::array<double, 3> levelShelf{};
        std::array<double, 1> nonlinearSourceHighPass{};
        std::array<double, 2> nonlinearOutputHighPass{};
        primitives::filters::BiquadState presencePeak{};
        primitives::filters::BiquadState presenceNotch{};
        primitives::filters::BiquadState levelBell{};
    };

    void PrepareFilters(double sampleRate) noexcept;
    void UpdateInputLevel(double peak) noexcept;
    [[nodiscard]] double ProcessLinearAddition(double input, Channel& state, double levelLoss) noexcept;
    [[nodiscard]] double ProcessOddHarmonics(double input, Channel& state) noexcept;
    [[nodiscard]] double ProcessChannelSample(
        double input, Channel& state, double amount, double baselineMix, double levelLoss) noexcept;

    primitives::ParameterRamp amount_;
    // Filter histories are independent per channel; input level is linked across selected channels.
    std::array<Channel, 2> channels_{};
    double baselineHighPassCoefficient_{};
    double lowerShelfCoefficient_{};
    double upperShelfCoefficient_{};
    double levelShelfCoefficient_{};
    double nonlinearSourceHighPassCoefficient_{};
    double nonlinearOutputHighPassCoefficient_{};
    primitives::filters::BiquadCoefficients presencePeakCoefficients_{};
    primitives::filters::BiquadCoefficients presenceNotchCoefficients_{};
    primitives::filters::BiquadCoefficients levelBellCoefficients_{};
    double peakReleaseCoefficient_{};
    std::size_t peakHoldSamples_{};
    std::size_t peakHoldRemaining_{};
    double inputPeak_{};
    double linearLevelGain_{ 1.0 };
    inline static constexpr PolishAirConfig kConfig{};
};

} // namespace consolidator::audio::algorithms
