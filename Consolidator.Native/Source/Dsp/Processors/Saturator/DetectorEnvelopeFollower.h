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

    // Applies detector EQ, rectifies the signal and updates the smoothed envelope.
    [[nodiscard]] double ProcessSample(double input) noexcept;


    bool ApplyParameter(
        const core::StatePath& route,
        const ParameterVariant& value,
        std::size_t depth);

    void CommitRuntimeUpdates();

    void SetAttackMs(double attackMs);
    void SetReleaseMs(double releaseMs);

    [[nodiscard]] const Equalizer& GetEqualizer() const noexcept
    {
        return filters_;
    }

private:
    void RecalculateTimeCoefficients() noexcept;

    Equalizer filters_{detail::ElementKind::SaturatorDetectorFilter};

    DetectorEnvelopeFollowerSettings settings_;

    double sampleRate_ = core::settings::kDefaultSampleRate;
    double attackCoefficient_ = 0.0;
    double releaseCoefficient_ = 0.0;
    double envelope_ = 0.0;
};

} // namespace consolidator::dsp
