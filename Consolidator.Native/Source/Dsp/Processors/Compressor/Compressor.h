#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "Core/State/CompressorState.h"
#include "Dsp/Processors/Compressor/RmsDetector.h"
#include "Dsp/Processors/DspDevice.h"
#include "Dsp/Processors/Equalizer/Equalizer.h"

namespace consolidator::dsp
{

struct CompressorRuntimeState
{
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

    [[nodiscard]] const CompressorState& GetState() const noexcept
    {
        return state_;
    }

    [[nodiscard]] const CompressorRuntimeState& GetRuntimeState() const noexcept
    {
        return runtimeState_;
    }

    bool WriteParameter(
        const core::StatePath& route,
        const ParameterValue& value,
        std::size_t depth) override;

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

    void ReadState(const core::StatePath& path, core::StateSnapshot& snapshot) const override
    {
        AppendParameter(path, snapshot, core::StatePath{DeviceId::Compressor, ParameterId::Threshold}, state_.thresholdDb);
        AppendParameter(path, snapshot, core::StatePath{DeviceId::Compressor, ParameterId::Ratio}, state_.ratio);
        AppendParameter(path, snapshot, core::StatePath{DeviceId::Compressor, ParameterId::Attack}, state_.attackMs);
        AppendParameter(path, snapshot, core::StatePath{DeviceId::Compressor, ParameterId::Release}, state_.releaseMs);
        AppendParameter(path, snapshot, core::StatePath{DeviceId::Compressor, ParameterId::Gain}, state_.outputDb);
        AppendParameter(path, snapshot, core::StatePath{DeviceId::Compressor, ParameterId::Mix}, state_.mix);
        AppendParameter(path, snapshot, core::StatePath{DeviceId::Compressor, ParameterId::Bypass}, state_.bypass);
        detectorEqualizer_.ReadAtRoute(path, snapshot, DeviceId::Compressor, RouteNodeId::Detector);
    }

private:
    bool WriteOwnParameter(
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

    CompressorState state_;
    CompressorRuntimeState runtimeState_;
    CompressorMeterState meterState_;

    Equalizer detectorEqualizer_{detail::ElementKind::CompressorDetectorFilter};
    RmsDetector rmsDetector_;
};

} // namespace consolidator::dsp
