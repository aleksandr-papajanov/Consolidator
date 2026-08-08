#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/Equalizer/Equalizer.h"
#include "Dsp/Utilities/TimeCoefficient.h"

namespace consolidator::dsp
{

struct EnvelopeDetectorSettings
{
    double attackMs = core::settings::DetectorDefaults::kDefaultAttackMs;
    double releaseMs = core::settings::DetectorDefaults::kDefaultReleaseMs;
};

class EnvelopeDetector
{
public:
    EnvelopeDetector(
        SaturatorDetectorFilterId lowShelfId,
        SaturatorDetectorFilterId bellId);

    void Prepare(double sampleRate);
    void Reset() noexcept;

    [[nodiscard]] double ProcessSample(double input) noexcept;


    bool ApplyParameter(
        const ParameterRoute& route,
        const ParameterValue& value,
        std::size_t depth);

    void SetAttackMs(double attackMs);
    void SetReleaseMs(double releaseMs);

private:
    static constexpr double kMinimumTimeMs = 0.01;

    void RecalculateTimeCoefficients() noexcept;

    Equalizer filters_{detail::ElementKind::SaturatorDetectorFilter};

    EnvelopeDetectorSettings settings_;

    double sampleRate_ = core::settings::kDefaultSampleRate;
    double attackCoefficient_ = 0.0;
    double releaseCoefficient_ = 0.0;
    double envelope_ = 0.0;
};

} // namespace consolidator::dsp
