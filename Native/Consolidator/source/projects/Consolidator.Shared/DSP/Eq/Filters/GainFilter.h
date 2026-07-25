#pragma once

#include "../IEqFilter.h"
#include "FilterSettings.h"
#include "../../SmoothedParameter.h"
#include "../../../Helpers/NumericHelper.h"
#include "../../../Settings/AudioOptions.h"

namespace consolidator::dsp {

class GainFilter final : public IEqFilter {
public:
    explicit GainFilter(const GainFilterSettings& settings)
        : gainDb(
            settings.gainDb,
            settings::AudioOptions::ParameterSmoothingSamples(settings.sampleRate)
        ) {}

    double ProcessSample(double input) override {
        return input * helpers::NumericHelper::DecibelsToMagnitude(
            gainDb.Next().value
        );
    }

    double GetMagnitudeDb(double frequencyHz) const override {
        (void)frequencyHz;
        return gainDb.Target();
    }

    double GetPhaseRadians(double frequencyHz) const override {
        (void)frequencyHz;
        return 0.0;
    }

    void Reset() override {}

    void UpdateSettings(const GainFilterSettings& settings) {
        gainDb.SetTarget(settings.gainDb);
    }

private:
    SmoothedParameter gainDb;
};

} // namespace consolidator::dsp
