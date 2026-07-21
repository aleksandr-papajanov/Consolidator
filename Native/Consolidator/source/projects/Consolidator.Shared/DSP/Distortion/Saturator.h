#pragma once

#include "DSP/IDspDevice.h"
#include "DSP/SmoothedParameter.h"
#include "Helpers/NumericHelper.h"
#include "Settings/AudioOptions.h"
#include "Settings/SaturatorOptions.h"

#include <cmath>

namespace consolidator::dsp {

struct SaturatorSettings {
    double saturation = settings::SaturatorOptions::DefaultSaturation;
    double sampleRate = settings::AudioOptions::DefaultSampleRateHz;
};

class Saturator final : public IDspDevice {
public:
    explicit Saturator(SaturatorSettings configuration)
        : saturation(helpers::NumericHelper::Clamp(
              configuration.saturation,
              settings::SaturatorOptions::MinimumSaturation,
              settings::SaturatorOptions::MaximumSaturation),
              settings::AudioOptions::ParameterSmoothingSamples(configuration.sampleRate)) {}

    double ProcessSample(double input) override {
        const auto value = saturation.Next().value;
        if (value <= 0.0) return input;
        const auto drive = 1.0 + value * (settings::SaturatorOptions::MaximumDrive - 1.0);
        const auto shaped = std::tanh(input * drive) / std::tanh(drive);
        return input + value * (shaped - input);
    }

    void Reset() override {}

    void UpdateSettings(const SaturatorSettings& settings) {
        saturation.SetTarget(helpers::NumericHelper::Clamp(
            settings.saturation,
            settings::SaturatorOptions::MinimumSaturation,
            settings::SaturatorOptions::MaximumSaturation));
    }

private:
    SmoothedParameter saturation;
};

} // namespace consolidator::dsp
