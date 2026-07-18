#pragma once

#include "BiquadFilter.h"
#include "FilterSettings.h"

namespace consolidator::dsp {

class BiquadBellFilter final : public BiquadFilter {
public:
    explicit BiquadBellFilter(BellFilterSettings settings)
        : settings(settings), coefficients(CreateCoefficients(settings)) {}

    double ProcessSample(double input) override {
        return state.ProcessSample(input, coefficients);
    }

    double GetMagnitudeDb(double frequencyHz) const override {
        return BiquadFilter::GetMagnitude(coefficients, frequencyHz, settings.sampleRate);
    }

    double GetPhaseRadians(double frequencyHz) const override {
        return BiquadFilter::GetPhase(coefficients, frequencyHz, settings.sampleRate);
    }

    void Reset() override {
        state.Reset();
    }

private:
    static Coefficients CreateCoefficients(const BellFilterSettings& settings) {
        const auto parameters = Sanitize(settings.frequencyHz, settings.q, settings.sampleRate);
        const double amplitude = helpers::NumericHelper::GainDbToBiquadAmplitude(settings.gainDb);
        const double omega = 2.0 * std::numbers::pi * parameters.frequencyHz / parameters.sampleRate;
        const double alpha = std::sin(omega) / (2.0 * parameters.q);
        return {
            1.0 + alpha * amplitude, -2.0 * std::cos(omega), 1.0 - alpha * amplitude,
            1.0 + alpha / amplitude, -2.0 * std::cos(omega), 1.0 - alpha / amplitude
        };
    }

    BellFilterSettings settings;
    Coefficients coefficients;
    State state;
};

} // namespace consolidator::dsp
