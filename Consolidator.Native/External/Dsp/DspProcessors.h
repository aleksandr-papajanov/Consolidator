#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "DspBiquad.h"
#include "DspParameterRamp.h"
#include "DspProcessor.h"

namespace consolidator::max::dsp
{

class DspGainRamp final
{
public:
    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    void SetTargetDb(float targetDb) noexcept;
    void Process(
        DspAudioBuffer& audio,
        DspChannelMode channelMode) noexcept;

    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    DspParameterRamp gain_{ 1.0 };
};

class DetectorProcessor final : public DspProcessor
{
public:
    explicit DetectorProcessor(std::size_t filterOffset) noexcept;

    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    static constexpr std::size_t kFilterCount = 2;

    std::size_t filterOffset_{};
    std::array<DspBiquadCascade, kFilterCount> filters_{};
    std::vector<double> left_;
    std::vector<double> right_;
};

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
    bool enabled_{};
    bool slowGateOpen_{};
    bool hasSlowEnergyEstimate_{};
    bool hasFastEnergyEstimate_{};
    bool parametersInitialized_{};
};

class InputProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

    [[nodiscard]] bool RequiresProcessing() const noexcept;
    [[nodiscard]] bool IsLevelerEnabled() const noexcept;
    [[nodiscard]] float SlowInputLevelDb() const noexcept;
    [[nodiscard]] float EffectiveInputLevelDb() const noexcept;
    [[nodiscard]] float FastInputLevelDb() const noexcept;

private:
    void ProcessWidth(DspProcessingFrame& frame) noexcept;

    DetectorProcessor levelerDetector_{ 0 };
    LevelerProcessor leveler_;
    DspParameterRamp width_{ 100.0 };
};

class SaturationProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    DspParameterRamp drive_;
    DspParameterRamp curve_{ 0.5 };
    bool split_{};
};

class SaturatorProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;
    void SynchronizeInactiveState() noexcept;

private:
    DetectorProcessor detector_{ 2 };
    SaturationProcessor saturation_;
    DspGainRamp output_;
};

class TransientShaperProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    DspParameterRamp attack_;
    DspParameterRamp sustain_;
};

class CompressionProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    DspParameterRamp amount_;
    std::uint32_t character_{};
};

class CompressorProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;
    void SynchronizeInactiveState() noexcept;

private:
    DetectorProcessor detector_{ 4 };
    TransientShaperProcessor transientShaper_;
    CompressionProcessor compression_;
    DspGainRamp output_;
};

class EqualizerProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    static constexpr std::size_t kBankCount = 7;
    static constexpr std::size_t kFilterCount = 7;

    std::array<DspBiquadCascade, kBankCount * kFilterCount> filters_{};
};

class PolishThickProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    DspParameterRamp amount_;
};

class PolishAirProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    DspParameterRamp amount_;
};

class PolishProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;
    void SynchronizeInactiveState() noexcept;

private:
    PolishThickProcessor thick_;
    PolishAirProcessor air_;
    std::vector<double> thickLeft_;
    std::vector<double> thickRight_;
    std::vector<double> airLeft_;
    std::vector<double> airRight_;
};

class LimiterProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    DspParameterRamp targetDb_{ -1.0 };
};

class OutputProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    DspGainRamp level_;
    LimiterProcessor limiter_;
    bool limiterEnabled_{};
};

} // namespace consolidator::max::dsp
