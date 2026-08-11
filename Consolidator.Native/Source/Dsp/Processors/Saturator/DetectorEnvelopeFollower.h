#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Equalizer.h"
#include "Dsp/Utilities/TimeCoefficient.h"

namespace consolidator::dsp
{

struct DetectorEnvelopeFollowerSettings
{
    double attackMs = core::settings::DetectorDefaults::kDefaultAttackMs;
    double releaseMs = core::settings::DetectorDefaults::kDefaultReleaseMs;
};

// Filters a detector signal and tracks its envelope with attack/release smoothing.
class DetectorEnvelopeFollower
{
public:
    DetectorEnvelopeFollower(
        SaturatorDetectorFilterId lowShelfId,
        SaturatorDetectorFilterId bellId);

    void Prepare(double sampleRate);
    void Reset() noexcept;
    // Routes reset requests to the detector envelope or its filters.
    bool Reset(
        const core::StatePath& path,
        std::size_t depth) noexcept;

    // Applies detector EQ, rectifies the signal and updates the smoothed envelope.
    [[nodiscard]] double ProcessSample(double input) noexcept;


    bool ApplyParameter(
        const core::StatePath& route,
        const ParameterVariant& value,
        std::size_t depth);

    bool ApplyProcessingStateAtDepth(
        const core::StatePath& target,
        bool active,
        std::size_t depth);

    bool ApplyMonitoringState(
        const core::StatePath& target,
        bool enabled,
        std::size_t depth);

    void CommitRuntimeUpdates();

    void SetAttackMs(double attackMs);
    void SetReleaseMs(double releaseMs);

    [[nodiscard]] const Equalizer& GetEqualizer() const noexcept
    {
        return filters_;
    }

    [[nodiscard]] double GetMonitoringSample() const noexcept
    {
        return monitoringSample_;
    }

    [[nodiscard]] bool IsListening() const noexcept
    {
        return listen_;
    }

private:
    void RecalculateTimeCoefficients() noexcept;

    Equalizer filters_{detail::ElementKind::SaturatorDetectorFilter};

    DetectorEnvelopeFollowerSettings settings_;

    double sampleRate_ = core::settings::kDefaultSampleRate;
    double attackCoefficient_ = 0.0;
    double releaseCoefficient_ = 0.0;
    double envelope_ = 0.0;
    double monitoringSample_ = 0.0;
    bool listen_ = false;
};

} // namespace consolidator::dsp
