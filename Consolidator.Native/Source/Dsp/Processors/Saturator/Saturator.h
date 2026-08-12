#pragma once

#include <array>
#include <cstddef>

#include "Dsp/Processors/DspDevice.h"
#include "Dsp/Processors/Saturator/DetectorEnvelopeFollower.h"

namespace consolidator::dsp
{

struct SaturatorRuntimeState
{
    float drive = 1.0f;
    float outputDb = 0.0f;
    float mix = 1.0f;
    float detectorAmountTarget = 1.0f;
    double driveLinear = 1.0;
    double outputGainLinear = 1.0;
    double wetMix = 1.0;
    double dryMix = 0.0;
    double detectorAmount = 1.0;
    bool isNeutral = true;
};

struct SaturatorBlockTelemetry
{
    // RMS nonlinear residual divided by RMS linear reference, in percent.
    // This is not spectral THD.
    float distortionPercent = 0.0f;
};

// Applies envelope-driven nonlinear waveshaping with dry/wet mixing.
class Saturator final : public DspDevice
{
public:
    Saturator();

    void Prepare(
        double sampleRate,
        std::size_t channelCount) override;

    void Reset() noexcept;
    // Routes reset requests to the saturator or its detector filters.
    bool Reset(
        const core::StatePath& path,
        std::size_t depth) noexcept override;

    // Processes the block and applies detector modulation without audio-thread allocation.
    void Process(
        const double* inputLeft,
        const double* inputRight,
        double* outputLeft,
        double* outputRight,
        std::size_t frameCount) override;

    [[nodiscard]] const SaturatorRuntimeState& GetRuntimeState() const noexcept
    {
        return runtimeState_;
    }

    [[nodiscard]] const DetectorEnvelopeFollower& GetDetector(std::size_t channel) const noexcept
    {
        return detectors_[channel];
    }

    [[nodiscard]] SaturatorBlockTelemetry GetBlockTelemetry() const noexcept;
    void ResetBlockTelemetry() noexcept;
    void SetTelemetryEnabled(bool enabled) noexcept
    {
        telemetryEnabled_ = enabled;
    }

    // Routes updates to the saturator or its detector filters.
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

    // Recalculates derived drive, mix and detector runtime values once per batch.
    void CommitRuntimeUpdates() override;

    [[nodiscard]] bool IsNeutral() const noexcept override
    {
        return runtimeState_.isNeutral;
    }

private:
    static constexpr std::size_t kMaximumChannelCount = 2;

    bool ApplyOwnParameter(
        const core::StatePath& route,
        const ParameterVariant& value) override;
    void RecalculateRuntime() override;


    [[nodiscard]] double ProcessSample(
        double input,
        DetectorEnvelopeFollower& detector,
        double& linearReference,
        double& shaped) const noexcept;

    [[nodiscard]] double CalculateDriveModulation(
        double envelope) const noexcept;

    [[nodiscard]] double ApplyWaveshaper(
        double input,
        double drive) const noexcept;

    void SetDrive(float drive);
    void SetOutputDb(float outputDb);
    void SetMix(float mix);
    void SetDetectorAmount(float amount);

    SaturatorRuntimeState runtimeState_;

    std::size_t activeChannelCount_ = kMaximumChannelCount;
    double distortionResidualSumSquares_ = 0.0;
    double distortionLinearSumSquares_ = 0.0;
    std::size_t distortionSampleCount_ = 0;
    bool telemetryEnabled_ = true;

    std::array<DetectorEnvelopeFollower, kMaximumChannelCount> detectors_{
        DetectorEnvelopeFollower{
            SaturatorDetectorFilterId::Filter1,
            SaturatorDetectorFilterId::Filter2
        },
        DetectorEnvelopeFollower{
            SaturatorDetectorFilterId::Filter1,
            SaturatorDetectorFilterId::Filter2
        }
    };
};

} // namespace consolidator::dsp
