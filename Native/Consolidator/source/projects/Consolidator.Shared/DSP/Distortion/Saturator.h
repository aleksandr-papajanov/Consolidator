#pragma once

#include "DSP/IDspDevice.h"
#include "DSP/SmoothedParameter.h"
#include "Helpers/NumericHelper.h"
#include "Settings/AudioOptions.h"
#include "Settings/SaturatorOptions.h"
#include "Models/DetectorFilterState.h"
#include "DSP/Eq/Filters/BiquadBellFilter.h"
#include "DSP/Eq/DetectorFilterFactory.h"

#include <array>
#include <cmath>

namespace consolidator::dsp {

struct SaturatorSettings {
    double saturation = settings::SaturatorOptions::DefaultSaturation;
    double outputDb = settings::SaturatorOptions::DefaultOutputDb;
    std::array<models::DetectorFilterState, 2> detectorFilters{
        models::DetectorFilterState{ 1 }, models::DetectorFilterState{ 2 }
    };
    long detectorListen = 0;
    double sampleRate = settings::AudioOptions::DefaultSampleRateHz;
};

class Saturator final : public IDspDevice {
public:
    explicit Saturator(SaturatorSettings configuration)
        : saturation(
              configuration.saturation,
              settings::AudioOptions::ParameterSmoothingSamples(configuration.sampleRate)),
          outputGain(
              helpers::NumericHelper::DecibelsToMagnitude(configuration.outputDb),
              settings::AudioOptions::ParameterSmoothingSamples(configuration.sampleRate)),
          detectorFilters(CreateDetectorFilters(configuration.detectorFilters, configuration.sampleRate)),
          detectorListen(configuration.detectorListen) {
        for (std::size_t index = 0; index < detectorFilters.size(); ++index) {
            detectorActive[index] = IsDetectorActive(configuration.detectorFilters[index]);
        }
    }

    double ProcessSample(double input) override {
        const auto detectorInput = ProcessDetector(input);
        const auto wetInput = detectorListen > 0 ? detectorInput : input;
        const auto amount = helpers::NumericHelper::Clamp(saturation.Next().value, 0.0, 1.0);
        const auto drive = helpers::NumericHelper::DecibelsToMagnitude(
            amount * settings::SaturatorOptions::MaximumDriveDb);
        const auto shaped = std::tanh(wetInput * drive) / std::tanh(drive);
        const auto processed = (wetInput + amount * (shaped - wetInput)) * outputGain.Next().value;
        return processed;
    }

    void Reset() override {}

    void UpdateSettings(const SaturatorSettings& settings) {
        saturation.SetTarget(helpers::NumericHelper::Clamp(
            settings.saturation,
            settings::SaturatorOptions::MinimumSaturation,
            settings::SaturatorOptions::MaximumSaturation));
        outputGain.SetTarget(helpers::NumericHelper::DecibelsToMagnitude(
            helpers::NumericHelper::Clamp(
                settings.outputDb,
                settings::SaturatorOptions::MinimumOutputDb,
                settings::SaturatorOptions::MaximumOutputDb)));
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

    static bool IsDetectorActive(const models::DetectorFilterState& filter) {
        return !filter.bypass && std::abs(filter.gainDb) >= 1.0e-12;
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

    SmoothedParameter saturation;
    SmoothedParameter outputGain;
    std::array<BiquadBellFilter, 2> detectorFilters;
    std::array<bool, 2> detectorActive{ false, false };
    long detectorListen = 0;
};

} // namespace consolidator::dsp
