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
    bool bypass = false;
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
};

class Compressor final : public DspDevice
{
public:
    Compressor();

    void Prepare(double sampleRate, std::size_t channelCount);
    void Reset() noexcept;

    void Process(
        const double* input,
        double* output,
        std::size_t frameCount,
        std::size_t channelCount) override;

    [[nodiscard]] const CompressorRuntimeState& GetRuntimeState() const noexcept
    {
        return runtimeState_;
    }

    bool ApplyParameter(
        const core::StatePath& route,
        const ParameterValue& value,
        std::size_t depth) override;

    bool StageRuntimeUpdate(
        const core::StatePath& route,
        const ParameterValue& value) override;

    void CommitRuntimeUpdates() override;

    [[nodiscard]] float GetGainReductionDb() const noexcept
    {
        return meterState_.gainReductionDb.load(std::memory_order_relaxed);
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
        const ParameterValue& value) override;

    void RecalculateRuntime() override;
    void RecalculateAttackCoefficient() noexcept;
    void RecalculateReleaseCoefficient() noexcept;
    void RecalculateOutputGain();
    void RecalculateMix() noexcept;


    [[nodiscard]] double CalculateLinkedDetectorInput(
        const double* frame,
        std::size_t channelCount) noexcept;

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
    void SetBypass(bool bypass) noexcept;

    CompressorRuntimeState runtimeState_;
    CompressorMeterState meterState_;

    Equalizer detectorEqualizer_{detail::ElementKind::CompressorDetectorFilter};
    RmsDetector rmsDetector_;
};

} // namespace consolidator::dsp
