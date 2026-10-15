#pragma once

#include <array>
#include <cstddef>

#include "../../Primitives/ParameterRamp.h"
#include "../../Core/AudioBuffer.h"

namespace consolidator::audio::algorithms
{

struct TransientShaperConfig
{
    double sidechainHz{ 20 };
    double fastAttackSeconds{ 0.00345 };
    double fastReleaseSeconds{ 0.01664 };
    double slowAttackSeconds{ 0.00284 };
    double slowReleaseSeconds{ 0.568 };
    double contextSeconds{ 0.00889 };
    double riseDb{ 12.56 };
    double attackReleaseSeconds{ 0.01352 };
    double attackAdditionReleaseSeconds{ 0.06 };
    double attackAdditionMix{ 0.1 };
    double maskCloseSeconds{ 0.000025 };
    double sustainOpenSeconds{ 0.00114 };
    double peakReleaseSeconds{ 0.271 };
    double tailBoost{ 0.885 };
    double tailExponent{ 0.426 };
    double baseGainDb{ -4.55 };
    double steadyReturn{ 0.1224 };
    double attackBoost{ 4.736 };
    double strengthRangeDb{ 117.0 };
    double initialSlowLevelDb{ -20 };
};

// Relative-onset masks with signed gain controls and permanent two-stage limiting.
class TransientShaperProcessor final
{
public:
    void Prepare(const ProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetAmounts(double attack, double sustain) noexcept;
    void SetOutputGainDb(double gainDb) noexcept;
    void Process(AudioBuffer audio, ChannelMode mode = ChannelMode::Stereo) noexcept;

private:
    void PrepareSidechainFilter(double sampleRate) noexcept;
    void PrepareDetectorCoefficients(double sampleRate) noexcept;
    void UpdatePowerFollowers(double power) noexcept;
    void UpdateHeldPeak(double magnitude) noexcept;
    void UpdateMasks(double magnitude) noexcept;
    [[nodiscard]] double CalculateSustainStageGain(double sustain) const noexcept;
    [[nodiscard]] double CalculateAttackStageGain(double attack, double sustainGain) const noexcept;
    [[nodiscard]] double ProcessOutputSample(
        double input, std::size_t channel, double sustainGain, double attackGain, double outputGain) noexcept;
    [[nodiscard]] double FilterSidechainSample(double input, std::size_t channel) noexcept;

    inline static constexpr TransientShaperConfig kConfig{};
    primitives::ParameterRamp attackAmount_;
    primitives::ParameterRamp sustainAmount_;
    primitives::ParameterRamp outputGain_{ 1.0 };
    double outputGainDb_{};
    double outputHighpassCoefficient_{};
    std::array<std::array<double, 1>, 2> outputHighpassStates_{};
    std::array<std::array<double, 2>, 2> filterStates_{};
    double b0_{};
    double b1_{};
    double b2_{};
    double a1_{};
    double a2_{};

    // Prepared follower/mask coefficients; runtime processing never recompiles filters.
    double fastAttackCoefficient_{};
    double fastReleaseCoefficient_{};
    double slowAttackCoefficient_{};
    double slowReleaseCoefficient_{};
    double onsetAttackCoefficient_{};
    double onsetReleaseCoefficient_{};
    double contextCoefficient_{};
    double attackOpenCoefficient_{};
    double attackReleaseCoefficient_{};
    double attackAdditionDecay_{};
    double maskCloseCoefficient_{};
    double sustainOpenCoefficient_{};
    double peakDecay_{};
    double peakDecayDb_{};
    double riseNormalization_{};
    double baseGain_{};
    double initialSlowPower_{};

    // Linked histories stay warm at neutral amounts so live control changes reuse the same context.
    double fastPower_{};
    double slowPower_{};
    double onsetPower_{};
    double contextPower_{};
    double peak_{};
    double peakDb_{ -240.0 };
    double attackMask_{};
    double extendedAttackMask_{};
    double sustainMask_{};
};

} // namespace consolidator::audio::algorithms
