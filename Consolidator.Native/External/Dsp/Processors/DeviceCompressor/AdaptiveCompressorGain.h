#pragma once

#include <array>
#include <cstddef>
#include <vector>

#include "../../DspParameterRamp.h"
#include "../../DspProcessingContext.h"

namespace consolidator::max::dsp::processors
{

// Owns the single Output gain. Auto mode observes dry and pre-Output wet power.
// Prepare owns allocation; CaptureReference and Process use only prepared storage.
class AdaptiveCompressorGain final
{
public:
    void Prepare(const DspProcessingContext& context);
    void Reset(double outputGain) noexcept;
    void ResetMeasurement() noexcept;
    void SetControls(bool enabled, double outputGain) noexcept;
    [[nodiscard]] double OutputDb() const noexcept;
    void CaptureReference(DspAudioBuffer audio, DspChannelMode mode) noexcept;
    void Process(DspAudioBuffer audio, DspChannelMode mode) noexcept;

private:
    struct Coefficients
    {
        double b0{};
        double b1{};
        double b2{};
        double a1{};
        double a2{};
    };

    struct FilterState
    {
        double z1{};
        double z2{};
        [[nodiscard]] double Process(double input, const Coefficients& coefficients) noexcept;
        void ClearQuietState() noexcept;
    };

    struct ChannelState
    {
        FilterState shelf;
        FilterState highPass;
    };

    void PrepareWeighting(double sampleRate) noexcept;
    void FinishMeasurementInterval() noexcept;
    [[nodiscard]] double WeightedPower(double input, ChannelState& state) const noexcept;

    Coefficients shelf_;
    Coefficients highPass_;
    std::array<ChannelState, 2> referenceStates_{};
    std::array<ChannelState, 2> wetStates_{};
    std::vector<double> referencePower_;
    DspParameterRamp gain_{ 1.0 };
    std::size_t intervalSamples_{};
    std::size_t accumulatedSamples_{};
    std::size_t activeIntervals_{};
    std::size_t warmupIntervals_{};
    double powerCoefficient_{};
    double correctionCoefficient_{};
    double referenceSum_{};
    double wetSum_{};
    double referenceMeanPower_{};
    double wetMeanPower_{};
    double correctionDb_{};
    bool enabled_{ true };
};

} // namespace consolidator::max::dsp::processors
