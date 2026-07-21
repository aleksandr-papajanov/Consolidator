#pragma once

#include "../IDspDevice.h"
#include "../SmoothedParameter.h"
#include "../../Helpers/NumericHelper.h"
#include "../../Settings/AudioOptions.h"

namespace consolidator::dsp {

struct GainSettings {
    double gainDb = 0.0;
    double sampleRate = settings::AudioOptions::DefaultSampleRateHz;
};

class Gain final : public IDspDevice {
public:
    explicit Gain(GainSettings settings)
        : gainDb(settings.gainDb,
            settings::AudioOptions::ParameterSmoothingSamples(settings.sampleRate)) {}

    double ProcessSample(double input) override {
        return input * helpers::NumericHelper::DecibelsToMagnitude(gainDb.Next().value);
    }

    void Reset() override {}

    void UpdateSettings(const GainSettings& settings) {
        gainDb.SetTarget(settings.gainDb);
    }

private:
    SmoothedParameter gainDb;
};

} // namespace consolidator::dsp
