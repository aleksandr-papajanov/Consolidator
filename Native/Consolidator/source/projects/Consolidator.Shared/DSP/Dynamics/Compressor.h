#pragma once

#include "DSP/IDspDevice.h"
#include "DSP/SmoothedParameter.h"
#include "Helpers/NumericHelper.h"
#include "Settings/AudioOptions.h"
#include "Settings/CompressorOptions.h"

#include <algorithm>
#include <cmath>

namespace consolidator::dsp {

struct CompressorSettings {
    double attackMs = settings::CompressorOptions::DefaultAttackMs;
    double releaseMs = settings::CompressorOptions::DefaultReleaseMs;
    double thresholdDb = settings::CompressorOptions::DefaultThresholdDb;
    double ratio = settings::CompressorOptions::FixedRatio;
    double sampleRate = settings::AudioOptions::DefaultSampleRateHz;
};

class Compressor final : public IDspDevice {
public:
    explicit Compressor(CompressorSettings settings)
        : sampleRate(settings.sampleRate), ratio(settings.ratio),
          attackMs(settings.attackMs, SmoothingSamples(settings.sampleRate)),
          releaseMs(settings.releaseMs, SmoothingSamples(settings.sampleRate)),
          thresholdDb(settings.thresholdDb, SmoothingSamples(settings.sampleRate)),
          attackCoefficient(TimeCoefficient(settings.attackMs)),
          releaseCoefficient(TimeCoefficient(settings.releaseMs)) {}

    double ProcessSample(double input) override {
        const auto attack = attackMs.Next();
        const auto release = releaseMs.Next();
        const auto threshold = thresholdDb.Next().value;
        if (attack.changed) attackCoefficient = TimeCoefficient(attack.value);
        if (release.changed) releaseCoefficient = TimeCoefficient(release.value);
        const auto level = std::abs(input);
        const auto coefficient = level > envelope ? attackCoefficient : releaseCoefficient;
        envelope = coefficient * envelope + (1.0 - coefficient) * level;
        const auto levelDb = helpers::NumericHelper::MagnitudeToDecibels(envelope);
        if (levelDb <= threshold) return input;
        const auto reductionDb = -(levelDb - threshold) * (1.0 - 1.0 / ratio);
        return input * helpers::NumericHelper::DecibelsToMagnitude(reductionDb);
    }

    void Reset() override { envelope = 0.0; }

    void UpdateSettings(const CompressorSettings& settings) {
        attackMs.SetTarget(settings.attackMs);
        releaseMs.SetTarget(settings.releaseMs);
        thresholdDb.SetTarget(settings.thresholdDb);
    }

private:
    static std::size_t SmoothingSamples(double sampleRate) {
        return settings::AudioOptions::ParameterSmoothingSamples(sampleRate);
    }

    double TimeCoefficient(double milliseconds) const {
        const auto seconds = std::max(milliseconds, 0.001) * 0.001;
        return std::exp(-1.0 / (seconds * sampleRate));
    }

    double sampleRate;
    double ratio;
    SmoothedParameter attackMs;
    SmoothedParameter releaseMs;
    SmoothedParameter thresholdDb;
    double envelope = 0.0;
    double attackCoefficient = 0.0;
    double releaseCoefficient = 0.0;
};

} // namespace consolidator::dsp
