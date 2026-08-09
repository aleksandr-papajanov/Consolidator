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

class DetectorEnvelopeFollower
{
public:
    DetectorEnvelopeFollower(
        SaturatorDetectorFilterId lowShelfId,
        SaturatorDetectorFilterId bellId);

    void Prepare(double sampleRate);
    void Reset() noexcept;

    [[nodiscard]] double ProcessSample(double input) noexcept;


    bool WriteParameter(
        const core::StatePath& route,
        const ParameterValue& value,
        std::size_t depth);

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
