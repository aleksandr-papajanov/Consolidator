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

class DspAutoGain final
{
public:
    void Prepare(const DspProcessingContext& context) noexcept;
    void Reset() noexcept;
    [[nodiscard]] double Process(double inputEnergy, double outputEnergy, double activity) noexcept;

private:
    double inputEnergy_{};
    double outputEnergy_{};
    double correctionDb_{};
    double energyCoefficient_{};
    double gainCoefficient_{};
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
    struct SplitState
    {
        double lowLeft{};
        double lowRight{};
        double midLeft{};
        double midRight{};
    };

    DspParameterRamp drive_;
    DspParameterRamp curve_{ 0.5 };
    DspParameterRamp splitMix_;
    DspAutoGain autoGain_;
    SplitState splitState_{};
    double detectorEnvelope_{};
    double envelopeReleaseCoefficient_{};
    std::array<double, 8> previousSamples_{};
    double envelopeWeight_{};
    double lowCrossoverCoefficient_{};
    double highCrossoverCoefficient_{};
};

class SaturatorProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

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
    DspParameterRamp activeMix_;
    double fastEnvelope_{};
    double slowEnvelope_{};
    double sustainEnvelope_{};
    double sustainCoefficient_{};
    DspAutoGain autoGain_;
    double detectorEnergy_{};
    double detectorCoefficient_{};
    bool detectorGateOpen_{};
    double gainDb_{};
    double fastAttackCoefficient_{};
    double slowAttackCoefficient_{};
    double gainAttackCoefficient_{};
    double gainReleaseCoefficient_{};
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
    DspParameterRamp activeMix_;
    DspParameterRamp attackSeconds_{ 0.025 };
    DspParameterRamp releaseSeconds_{ 0.060 };
    double sampleRate_{ 48000.0 };
    double detectorEnergy_{};
    double referenceEnergy_{};
    double inputEnergy_{};
    double outputEnergy_{};
    double reductionDb_{};
    double makeupDb_{};
    double reductionDuration_{};
    double measuredReductionDb_{};
    double reductionCorrectionDb_{};
    double detectorCoefficient_{};
    double referenceCoefficient_{};
    double meterCoefficient_{};
    double makeupCoefficient_{};
    bool referenceInitialized_{};
};

class CompressorProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    DetectorProcessor detector_{ 4 };
    DetectorProcessor compressionDetector_{ 4 };
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
    struct Channel
    {
        std::array<double, 2> source{};
        std::array<double, 2> highPass{};
        std::array<double, 2> lowPass{};
        double previousSample{};
    };

    DspParameterRamp amount_;
    std::array<Channel, 2> channels_{};
    double sourceCoefficient_{};
    double highPassCoefficient_{};
    double lowPassCoefficient_{};
    double energyCoefficient_{};
    double energy_{};
    double energyWeight_{};
};

class PolishAirProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    struct Channel
    {
        std::array<double, 2> sourceHighPass{};
        std::array<double, 2> sourceLowPass{};
        std::array<double, 2> outputHighPass{};
        std::array<double, 1> shelf{};
        double previousSample{};
    };

    DspParameterRamp amount_;
    std::array<Channel, 2> channels_{};
    double sourceHighPassCoefficient_{};
    double sourceLowPassCoefficient_{};
    double outputHighPassCoefficient_{};
    double shelfCoefficient_{};
    double energyCoefficient_{};
    double fastEnergyCoefficient_{};
    double attenuationAttackCoefficient_{};
    double attenuationReleaseCoefficient_{};
    double bandEnergy_{};
    double mainEnergy_{};
    double fastBandEnergy_{};
    double energyWeight_{};
    double fastEnergyWeight_{};
    double attenuation_{ 1.0 };
};

class PolishProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;

private:
    PolishThickProcessor thick_;
    PolishAirProcessor air_;
};

class LimiterProcessor final : public DspProcessor
{
public:
    void Prepare(const DspProcessingContext& context) noexcept override;
    void Reset() noexcept override;
    void SetParameters(const DspSnapshot& snapshot) noexcept override;
    void Process(DspProcessingFrame& frame) noexcept override;
    [[nodiscard]] bool RequiresProcessing() const noexcept;

private:
    DspParameterRamp targetDb_{ -1.0 };
    double gain_{ 1.0 };
    double releaseCoefficient_{};
    bool enabled_{};
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
};

} // namespace consolidator::max::dsp
