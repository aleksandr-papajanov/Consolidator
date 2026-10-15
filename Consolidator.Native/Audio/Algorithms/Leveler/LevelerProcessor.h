#pragma once

#include <cstddef>

#include "../../Primitives/ParameterRamp.h"
#include "../../Primitives/Filters/KWeighting.h"
#include "../../Core/AudioBuffer.h"

namespace consolidator::audio::algorithms
{

class LevelerProcessor final
{
public:
    void Prepare(const ProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetControls(double levelDb, double targetDb, bool enabled) noexcept;
    void Process(AudioBuffer audio, DetectorBuffer detector, ChannelMode mode) noexcept;

    [[nodiscard]] double SlowGainDb() const noexcept;
    [[nodiscard]] double FastCorrectionDb() const noexcept;

private:
    struct GainTargets
    {
        double effectiveDb;
        double slowDb;
        double fastDb;
    };

    [[nodiscard]] GainTargets CalculateAutomaticGain(
        DetectorBuffer detector, ChannelMode mode, std::size_t frameCount) noexcept;
    void ProcessManualGain(AudioBuffer audio, ChannelMode mode) noexcept;
    void ProcessAutomaticGain(AudioBuffer audio, ChannelMode mode, const GainTargets& targets) noexcept;

    static constexpr double kMinimumGainDb = -36.0;
    static constexpr double kMaximumGainDb = 36.0;
    static constexpr double kSilenceEnergy = 1.0e-12;

    primitives::filters::KWeighting weighting_;
    primitives::ParameterRamp targetDb_;
    primitives::ParameterRamp manualGainDb_;
    double slowEnergyCoefficient_{};
    double fastEnergyCoefficient_{};
    double slowTimeConstantSamples_{ 1.0 };
    double fastTimeConstantSamples_{ 1.0 };
    double smoothedSlowEnergy_{};
    double smoothedFastEnergy_{};
    double slowGainDb_{};
    double effectiveGainDb_{};
    double fastCorrectionDb_{};
    double gateEnergy_{};
    double gateCoefficient_{};
    std::size_t gateHoldSamples_{};
    std::size_t gateHoldRemaining_{};
    bool enabled_{};
    bool slowGateOpen_{};
    bool hasSlowEnergyEstimate_{};
    bool hasFastEnergyEstimate_{};
    bool parametersInitialized_{};
};

} // namespace consolidator::audio::algorithms
