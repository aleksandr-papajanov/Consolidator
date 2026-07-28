#pragma once

#include "DSP/IDspDevice.h"
#include "DSP/SmoothedParameter.h"
#include "Helpers/NumericHelper.h"
#include "Settings/AudioOptions.h"
#include "Settings/CompressorOptions.h"
#include "Models/DetectorFilterState.h"
#include "DSP/Eq/Filters/BiquadBellFilter.h"
#include "DSP/Eq/DetectorFilterFactory.h"

#include <algorithm>
#include <array>
#include <cmath>

namespace consolidator::dsp {

struct CompressorSettings {
    double attackMs = settings::CompressorOptions::DefaultAttackMs;
    double releaseMs = settings::CompressorOptions::DefaultReleaseMs;
    double thresholdDb = settings::CompressorOptions::DefaultThresholdDb;
    double outputDb = settings::CompressorOptions::DefaultOutputDb;
    double mix = settings::CompressorOptions::DefaultMix;
    std::array<models::DetectorFilterState, 2> detectorFilters{
        models::DetectorFilterState{ 1 }, models::DetectorFilterState{ 2 }
    };
    long detectorListen = 0;
    double sampleRate = settings::AudioOptions::DefaultSampleRateHz;
};

class Compressor final : public IDspDevice {
public:
    explicit Compressor(CompressorSettings settings)
        : sampleRate(settings.sampleRate),
          attackMs(settings.attackMs, SmoothingSamples(settings.sampleRate)),
          releaseMs(settings.releaseMs, SmoothingSamples(settings.sampleRate)),
          thresholdDb(settings.thresholdDb, SmoothingSamples(settings.sampleRate)),
          outputDb(settings.outputDb, SmoothingSamples(settings.sampleRate)),
          mix(settings.mix, SmoothingSamples(settings.sampleRate)),
          detectorFilters(CreateDetectorFilters(settings.detectorFilters, settings.sampleRate)),
          detectorListen(settings.detectorListen),
          attackCoefficient(TimeCoefficient(settings.attackMs)),
          releaseCoefficient(TimeCoefficient(settings.releaseMs)) {
        for (std::size_t index = 0; index < detectorFilters.size(); ++index) {
            detectorActive[index] = IsDetectorActive(settings.detectorFilters[index]);
        }
    }

    double ProcessSample(double input) override {
        const auto attack = attackMs.Next();
        const auto release = releaseMs.Next();
        const auto threshold = thresholdDb.Next().value;
        const auto outputGain = helpers::NumericHelper::DecibelsToMagnitude(outputDb.Next().value);
        const auto wet = helpers::NumericHelper::Clamp(mix.Next().value, 0.0, 1.0);
        const auto compressorInput = input;

        if (attack.changed) attackCoefficient = TimeCoefficient(attack.value);
        if (release.changed) releaseCoefficient = TimeCoefficient(release.value);

        const auto detectorInput = ProcessDetector(compressorInput);
        UpdateEnvelope(std::abs(detectorInput));
        lastGainReductionDb = CalculateGainReductionDb(threshold);

        const auto wetInput = detectorListen > 0 ? detectorInput : compressorInput;
        const auto compressed = wetInput * helpers::NumericHelper::DecibelsToMagnitude(lastGainReductionDb);
        return Mix(input, compressed * outputGain, wet);
    }

    DspDeviceTelemetry Telemetry() const noexcept override {
        return { lastGainReductionDb };
    }

    void Reset() override { envelope = 0.0; }

    void UpdateSettings(const CompressorSettings& settings) {
        attackMs.SetTarget(settings.attackMs);
        releaseMs.SetTarget(settings.releaseMs);
        thresholdDb.SetTarget(settings.thresholdDb);
        outputDb.SetTarget(settings.outputDb);
        mix.SetTarget(settings.mix);
        detectorListen = settings.detectorListen;
        for (std::size_t index = 0; index < detectorFilters.size(); ++index) {
            detectorActive[index] = IsDetectorActive(settings.detectorFilters[index]);
            detectorFilters[index].UpdateSettings(
                DetectorFilterFactory::Settings(settings.detectorFilters[index], settings.sampleRate));
        }
    }

private:
    double ProcessDetector(double input) {
        for (std::size_t index = 0; index < detectorFilters.size(); ++index) {
            if (detectorActive[index]) input = detectorFilters[index].ProcessSample(input);
        }
        return input;
    }

    void UpdateEnvelope(double level) {
        const auto coefficient = level > envelope ? attackCoefficient : releaseCoefficient;
        envelope = coefficient * envelope + (1.0 - coefficient) * level;
    }

    double CalculateGainReductionDb(double thresholdDb) const {
        const auto levelDb = helpers::NumericHelper::MagnitudeToDecibels(envelope);
        const auto excessDb = levelDb - thresholdDb;
        if (excessDb <= 0.0) return 0.0;
        return -excessDb * (1.0 - 1.0 / settings::CompressorOptions::FixedRatio);
    }

    static double Mix(double dry, double wet, double amount) {
        return dry + amount * (wet - dry);
    }

    static std::array<BiquadBellFilter, 2> CreateDetectorFilters(
        const std::array<models::DetectorFilterState, 2>& settings,
        double sampleRate
    ) {
        return {
            BiquadBellFilter(DetectorFilterFactory::Settings(settings[0], sampleRate)),
            BiquadBellFilter(DetectorFilterFactory::Settings(settings[1], sampleRate))
        };
    }

    static std::size_t SmoothingSamples(double sampleRate) {
        return settings::AudioOptions::ParameterSmoothingSamples(sampleRate);
    }

    static bool IsDetectorActive(const models::DetectorFilterState& filter) {
        return !filter.bypass && std::abs(filter.gainDb) >= 1.0e-12;
    }

    double TimeCoefficient(double milliseconds) const {
        const auto seconds = std::max(milliseconds, 0.001) * 0.001;
        return std::exp(-1.0 / (seconds * sampleRate));
    }

    double sampleRate;
    SmoothedParameter attackMs;
    SmoothedParameter releaseMs;
    SmoothedParameter thresholdDb;
    SmoothedParameter outputDb;
    SmoothedParameter mix;
    std::array<BiquadBellFilter, 2> detectorFilters;
    std::array<bool, 2> detectorActive{ false, false };
    long detectorListen = 0;
    double envelope = 0.0;
    double attackCoefficient = 0.0;
    double releaseCoefficient = 0.0;
    double lastGainReductionDb = 0.0;
};

} // namespace consolidator::dsp
