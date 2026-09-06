#pragma once

#include <cstddef>

#include "DspParameterRamp.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

class LevelerProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

    [[nodiscard]] bool RequiresProcessing() const noexcept;
    [[nodiscard]] bool IsEnabled() const noexcept;
    [[nodiscard]] float SlowInputLevelDb() const noexcept;
    [[nodiscard]] float EffectiveInputLevelDb() const noexcept;
    [[nodiscard]] float FastInputLevelDb() const noexcept;

private:
    static constexpr double kMinimumGainDb = -36.0;
    static constexpr double kMaximumGainDb = 36.0;
    static constexpr double kSilenceEnergy = 1.0e-12;

    DspParameterRamp targetDb_;
    DspParameterRamp inputGainDb_;
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

} // namespace consolidator::max::dsp
