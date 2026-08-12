#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "Dsp/Processors/Compressor/RmsDetector.h"
#include "Dsp/Processors/DspDevice.h"
#include "Dsp/Processors/Equalizer/Equalizer.h"

namespace consolidator::dsp
{

struct CompressorRuntimeState
{
    float thresholdDb = -12.0f;
    float ratio = 4.0f;
    float attackMs = 5.0f;
    float releaseMs = 100.0f;
    float outputDb = 0.0f;
    float mix = 1.0f;
    double sampleRate = core::settings::kDefaultSampleRate;
    double gainReductionDb = 0.0;
    double outputGainLinear = 1.0;
    double wetMix = 1.0;
    double dryMix = 0.0;
    double attackCoefficient = 0.0;
    double releaseCoefficient = 0.0;
    bool isNeutral = true;
};

struct CompressorMeterState
{
    std::atomic<float> gainReductionDb{0.0f};
    double attenuationSumSquares = 0.0;
    float minimumAttenuation = 1.0f;
    std::size_t attenuationSampleCount = 0;
    bool telemetryEnabled = true;
};

struct CompressorBlockTelemetry
{
    float gainReductionRmsDb = 0.0f;
    float gainReductionPeakDb = 0.0f;
};

// Measures linked detector level and applies time-smoothed dynamic gain reduction.
class Compressor final : public DspDevice
{
public:
    Compressor();

    void Prepare(
        double sampleRate,
        std::size_t channelCount) override;
    void Reset() noexcept;
    // Routes reset requests to the compressor or its detector filters.
    bool Reset(
        const core::StatePath& path,
        std::size_t depth) noexcept override;

    // Processes a block with feed-forward detection, smoothing and dry/wet mixing.
    void Process(
        const double* inputLeft,
        const double* inputRight,
        double* outputLeft,
        double* outputRight,
        std::size_t frameCount) override;

    [[nodiscard]] const CompressorRuntimeState& GetRuntimeState() const noexcept
    {
        return runtimeState_;
    }

    // Routes updates to the compressor or its detector equalizer.
    bool ApplyParameter(
        const core::StatePath& route,
        const ParameterVariant& value,
        std::size_t depth) override;

    bool ApplyProcessingStateAtDepth(
        const core::StatePath& target,
        bool active,
        std::size_t depth) override;

    bool ApplyMonitoringState(
        const core::StatePath& target,
        bool enabled,
        std::size_t depth) override;

    bool StageRuntimeUpdate(
        const core::StatePath& route,
        const ParameterVariant& value) override;

    // Recalculates all derived timing, gain and mix values after staged updates.
    void CommitRuntimeUpdates() override;

    [[nodiscard]] float GetGainReductionDb() const noexcept
    {
        return meterState_.gainReductionDb.load(std::memory_order_relaxed);
    }

    [[nodiscard]] CompressorBlockTelemetry GetBlockTelemetry() const noexcept;
    void ResetBlockTelemetry() noexcept;
    void SetTelemetryEnabled(bool enabled) noexcept
    {
        meterState_.telemetryEnabled = enabled;
    }

    [[nodiscard]] const Equalizer& GetDetectorEqualizer() const noexcept
    {
        return detectorEqualizer_;
    }

    [[nodiscard]] bool IsNeutral() const noexcept override
    {
        return runtimeState_.isNeutral;
    }

private:
    bool ApplyOwnParameter(
        const core::StatePath& route,
        const ParameterVariant& value) override;

    void RecalculateRuntime() override;
    void RecalculateAttackCoefficient() noexcept;
    void RecalculateReleaseCoefficient() noexcept;
    void RecalculateOutputGain();
    void RecalculateMix() noexcept;


    [[nodiscard]] double CalculateLinkedDetectorInput(
        double linkedInput) noexcept;

    [[nodiscard]] double MeasureLevelDb(double detectorInput) noexcept;

    [[nodiscard]] double CalculateTargetGainReductionDb(double inputLevelDb) const noexcept;

    [[nodiscard]] double UpdateGainReductionDb(double targetGainReductionDb) noexcept;

    [[nodiscard]] double ProcessSample(
        double input,
        double gainLinear) const noexcept;

    void SetThreshold(float thresholdDb) noexcept;
    void SetRatio(float ratio) noexcept;
    void SetAttack(float attackMs) noexcept;
    void SetRelease(float releaseMs) noexcept;
    void SetOutputDb(float outputDb);
    void SetMix(float mix) noexcept;

    CompressorRuntimeState runtimeState_;
    CompressorMeterState meterState_;

    Equalizer detectorEqualizer_{detail::ElementKind::CompressorDetectorFilter};
    RmsDetector rmsDetector_;
    bool detectorListen_ = false;
    double detectorMonitoringSample_ = 0.0;
};

} // namespace consolidator::dsp
