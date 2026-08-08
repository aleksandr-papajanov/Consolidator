#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Parameters/DspParameter.h"
#include "Dsp/Processors/Compressor/RmsDetector.h"
#include "Dsp/Processors/DspDevice.h"
#include "Dsp/Processors/Equalizer/Equalizer.h"

namespace consolidator::dsp
{

struct CompressorState
{
    DspParameter<float> thresholdDb{
        ParameterId::Threshold,
        static_cast<float>(core::settings::CompressorDefaults::kDefaultThresholdDb),
        static_cast<float>(core::settings::CompressorDefaults::kMinThresholdDb),
        static_cast<float>(core::settings::CompressorDefaults::kMaxThresholdDb)};

    DspParameter<float> ratio{
        ParameterId::Ratio,
        static_cast<float>(core::settings::CompressorDefaults::kDefaultRatio),
        static_cast<float>(core::settings::CompressorDefaults::kMinRatio),
        static_cast<float>(core::settings::CompressorDefaults::kMaxRatio)};

    DspParameter<float> attackMs{
        ParameterId::Attack,
        static_cast<float>(core::settings::CompressorDefaults::kDefaultAttackMs),
        static_cast<float>(core::settings::CompressorDefaults::kMinAttackMs),
        static_cast<float>(core::settings::CompressorDefaults::kMaxAttackMs)};

    DspParameter<float> releaseMs{
        ParameterId::Release,
        static_cast<float>(core::settings::CompressorDefaults::kDefaultReleaseMs),
        static_cast<float>(core::settings::CompressorDefaults::kMinReleaseMs),
        static_cast<float>(core::settings::CompressorDefaults::kMaxReleaseMs)};

    DspParameter<float> outputDb{
        ParameterId::Gain,
        static_cast<float>(core::settings::CompressorDefaults::kDefaultOutputDb),
        static_cast<float>(core::settings::CompressorDefaults::kMinOutputDb),
        static_cast<float>(core::settings::CompressorDefaults::kMaxOutputDb)};

    DspParameter<float> mix{
        ParameterId::Mix,
        static_cast<float>(core::settings::CompressorDefaults::kDefaultMix),
        static_cast<float>(core::settings::CompressorDefaults::kMinMix),
        static_cast<float>(core::settings::CompressorDefaults::kMaxMix)};
        
    DspParameter<bool> bypass{
        ParameterId::Bypass,
        false
    };
};

struct CompressorRuntimeState
{
    double outputGainLinear = 1.0;
    double wetMix = 1.0;
    double dryMix = 0.0;
    double attackCoefficient = 0.0;
    double releaseCoefficient = 0.0;
    bool isNeutral = true;
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

    bool ApplyParameter(
        const ParameterRoute& route,
        const ParameterValue& value,
        std::size_t depth) override;

    [[nodiscard]] float GetGainReductionDb() const noexcept
    {
        return displayedGainReductionDb_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] bool IsNeutral() const noexcept override
    {
        return runtimeState_.isNeutral;
    }

private:
    bool ApplyStateParameter(
        const ParameterRoute& route,
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

    Equalizer detectorEqualizer_{detail::ElementKind::CompressorDetectorFilter};

    RmsDetector rmsDetector_;

    double sampleRate_ = core::settings::kDefaultSampleRate;
    double gainReductionDb_ = 0.0;

    std::atomic<float> displayedGainReductionDb_{0.0f};
};

} // namespace consolidator::dsp
