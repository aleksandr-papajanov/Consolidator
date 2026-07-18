#pragma once

#include "BiquadFilter.h"
#include "FilterSettings.h"

namespace consolidator::dsp {

class BiquadHighShelfFilter final : public BiquadFilter {
public:
    explicit BiquadHighShelfFilter(HighShelfFilterSettings settings)
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
    static Coefficients CreateCoefficients(const HighShelfFilterSettings& settings) {
        const auto parameters = Sanitize(settings.frequencyHz, settings.q, settings.sampleRate);
        const double amplitude = helpers::NumericHelper::GainDbToBiquadAmplitude(settings.gainDb);
        const double omega = 2.0 * std::numbers::pi * parameters.frequencyHz / parameters.sampleRate;
        const double cosine = std::cos(omega);
        const double alpha = std::sin(omega) / (2.0 * parameters.q);
        const double beta = 2.0 * std::sqrt(amplitude) * alpha;
        return {
            amplitude * ((amplitude + 1.0) + (amplitude - 1.0) * cosine + beta),
            -2.0 * amplitude * ((amplitude - 1.0) + (amplitude + 1.0) * cosine),
            amplitude * ((amplitude + 1.0) + (amplitude - 1.0) * cosine - beta),
            (amplitude + 1.0) - (amplitude - 1.0) * cosine + beta,
            2.0 * ((amplitude - 1.0) - (amplitude + 1.0) * cosine),
            (amplitude + 1.0) - (amplitude - 1.0) * cosine - beta
        };
    }

    HighShelfFilterSettings settings;
    Coefficients coefficients;
    State state;
};

} // namespace consolidator::dsp
