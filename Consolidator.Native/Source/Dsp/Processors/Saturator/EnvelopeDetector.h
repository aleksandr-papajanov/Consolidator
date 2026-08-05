#pragma once

#include "Dsp/Parameters/ParameterChange.h"
#include "Dsp/Parameters/ParameterAddress.h"
#include "Dsp/Processors/Equalizer/Filters/BellFilter.h"
#include "Dsp/Processors/Equalizer/Filters/LowShelfFilter.h"

namespace consolidator::dsp
{

struct EnvelopeDetectorSettings
{
    double attackMs = 10.0;
    double releaseMs = 100.0;
};

class EnvelopeDetector
{
public:
    EnvelopeDetector(SaturatorDetectorFilterId lowShelfId,
                     SaturatorDetectorFilterId bellId);

    void Prepare(double sampleRate);
    void Reset() noexcept;

    [[nodiscard]] double ProcessSample(double input) noexcept;

    void ApplyParameterChange(const ParameterChange& change);

    void SetAttackMs(double attackMs);
    void SetReleaseMs(double releaseMs);

private:
    static constexpr double kMinimumTimeMs = 0.01;

    void RecalculateTimeCoefficients() noexcept;

    [[nodiscard]] static double CalculateTimeCoefficient(
        double timeMs,
        double sampleRate) noexcept;

    LowShelfFilter lowShelf_;
    BellFilter bell_;

    EnvelopeDetectorSettings settings_;

    double sampleRate_ = 48000.0;
    double attackCoefficient_ = 0.0;
    double releaseCoefficient_ = 0.0;
    double envelope_ = 0.0;
};

} // namespace consolidator::dsp