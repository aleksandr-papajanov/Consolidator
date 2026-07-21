#pragma once

#include "BiquadFilter.h"
#include "FilterSettings.h"
#include "../../SmoothedParameter.h"
#include "../../../Settings/AudioOptions.h"

namespace consolidator::dsp {

class BiquadHighShelfFilter final : public BiquadFilter {
public:
    explicit BiquadHighShelfFilter(HighShelfFilterSettings settings)
        : sampleRate(settings.sampleRate),
          frequencyHz(settings.frequencyHz, SmoothingSamples(settings.sampleRate)),
          q(settings.q, SmoothingSamples(settings.sampleRate)),
          gainDb(settings.gainDb, SmoothingSamples(settings.sampleRate)),
          coefficients(CreateCoefficients(settings)) {}

    double ProcessSample(double input) override {
        const auto frequency = frequencyHz.Next();
        const auto quality = q.Next();
        const auto gain = gainDb.Next();
        if (frequency.changed || quality.changed || gain.changed) {
            coefficients = CreateCoefficients({ frequency.value, quality.value, gain.value, sampleRate });
        }
        return state.ProcessSample(input, coefficients);
    }

    double GetMagnitudeDb(double frequencyHz) const override {
        return BiquadFilter::GetMagnitude(TargetCoefficients(), frequencyHz, sampleRate);
    }

    double GetPhaseRadians(double frequencyHz) const override {
        return BiquadFilter::GetPhase(TargetCoefficients(), frequencyHz, sampleRate);
    }

    void Reset() override {
        state.Reset();
    }

    void UpdateSettings(const HighShelfFilterSettings& settings) {
        frequencyHz.SetTarget(settings.frequencyHz);
        q.SetTarget(settings.q);
        gainDb.SetTarget(settings.gainDb);
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

    static std::size_t SmoothingSamples(double sampleRate) {
        return settings::AudioOptions::ParameterSmoothingSamples(sampleRate);
    }

    Coefficients TargetCoefficients() const {
        return CreateCoefficients({ frequencyHz.Target(), q.Target(), gainDb.Target(), sampleRate });
    }

    double sampleRate;
    SmoothedParameter frequencyHz;
    SmoothedParameter q;
    SmoothedParameter gainDb;
    Coefficients coefficients;
    State state;
};

} // namespace consolidator::dsp
