#pragma once

#include <atomic>
#include <cstddef>
#include <cstdint>

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Compressor/CompressorSidechain.h"
#include "Dsp/Processors/Compressor/RmsDetector.h"
#include "Dsp/Processors/IDspDevice.h"

namespace consolidator::dsp
{

struct CompressorState
{
    float thresholdDb = static_cast<float>(core::settings::CompressorDefaults::kDefaultThresholdDb);
    float ratio = static_cast<float>(core::settings::CompressorDefaults::kDefaultRatio);
    float attackMs = static_cast<float>(core::settings::CompressorDefaults::kDefaultAttackMs);
    float releaseMs = static_cast<float>(core::settings::CompressorDefaults::kDefaultReleaseMs);
    float outputDb = static_cast<float>(core::settings::CompressorDefaults::kDefaultOutputDb);
    float mix = static_cast<float>(core::settings::CompressorDefaults::kDefaultMix);
    bool bypass = false;
};

struct CompressorRuntime
{
    double outputGainLinear = 1.0;
    double wetMix = 1.0;
    double dryMix = 0.0;
    double attackCoefficient = 0.0;
    double releaseCoefficient = 0.0;
};

class Compressor final : public IDspDevice
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

    void ApplyParameterChange(const ParameterChange& change) override;

    [[nodiscard]] DeviceId GetDeviceId() const noexcept override
    {
        return DeviceId::Compressor;
    }

    [[nodiscard]] detail::ElementKind GetElementKind() const noexcept override
    {
        return detail::ElementKind::Device;
    }

    [[nodiscard]] std::uint8_t GetElementIndex() const noexcept override
    {
        return 0;
    }

    [[nodiscard]] const CompressorState& GetState() const noexcept
    {
        return state_;
    }

    [[nodiscard]] float GetGainReductionDb() const noexcept
    {
        return displayedGainReductionDb_.load(std::memory_order_relaxed);
    }

    [[nodiscard]] bool IsNeutral() const noexcept override
    {
        return state_.bypass
            || (state_.thresholdDb >= 0.0f
                && state_.ratio <= 1.0f
                && state_.outputDb == 0.0f);
    }

private:
    static constexpr double kSoftKneeWidthDb = 6.0;
    static constexpr double kMinimumLevelLinear = 1.0e-12;
    static constexpr double kMinimumGainReductionDb = -60.0;

    static constexpr float kMinimumAttackMs = 0.01f;
    static constexpr float kMaximumAttackMs = 2000.0f;

    static constexpr float kMinimumReleaseMs = 0.01f;
    static constexpr float kMaximumReleaseMs = 10000.0f;

    static constexpr float kMinimumRatio = 1.0f;
    static constexpr float kMaximumRatio = 100.0f;

    static constexpr float kMinimumThresholdDb = -60.0f;
    static constexpr float kMaximumThresholdDb = 0.0f;

    static constexpr float kMinimumOutputDb = -24.0f;
    static constexpr float kMaximumOutputDb = 24.0f;

    static constexpr float kMinimumMix = 0.0f;
    static constexpr float kMaximumMix = 1.0f;

    void RecalculateRuntime();
    void RecalculateAttackCoefficient() noexcept;
    void RecalculateReleaseCoefficient() noexcept;
    void RecalculateOutputGain();
    void RecalculateMix() noexcept;

    void ApplyCompressorParameter(const ParameterChange& change);
    void ApplyDetectorParameter(const ParameterChange& change);

    [[nodiscard]] bool IsDetectorParameter(const ParameterChange& change) const noexcept;

    [[nodiscard]] double CalculateLinkedDetectorInput(const double* frame, std::size_t channelCount) noexcept;

    [[nodiscard]] double MeasureLevelDb(double detectorInput) noexcept;

    [[nodiscard]] double CalculateTargetGainReductionDb(double inputLevelDb) const noexcept;

    [[nodiscard]] double UpdateGainReductionDb(double targetGainReductionDb) noexcept;

    [[nodiscard]] double ProcessSample(double input, double gainLinear) const noexcept;

    [[nodiscard]] static double CalculateTimeCoefficient(double timeMs, double sampleRate) noexcept;

    void SetThreshold(float thresholdDb) noexcept;
    void SetRatio(float ratio) noexcept;
    void SetAttack(float attackMs) noexcept;
    void SetRelease(float releaseMs) noexcept;
    void SetOutputDb(float outputDb);
    void SetMix(float mix) noexcept;
    void SetBypass(bool bypass) noexcept;

    CompressorState state_;
    CompressorRuntime runtime_;

    CompressorSidechain sidechain_{
        CompressorDetectorFilterId::Filter1,
        CompressorDetectorFilterId::Filter2};

    RmsDetector rmsDetector_;

    double sampleRate_ = core::settings::kDefaultSampleRate;
    double gainReductionDb_ = 0.0;

    std::atomic<float> displayedGainReductionDb_{0.0f};
};

} // namespace consolidator::dsp