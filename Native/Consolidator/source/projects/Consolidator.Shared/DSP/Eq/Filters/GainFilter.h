#pragma once

#include "FilterSettings.h"
#include "../IEqFilter.h"
#include "../../../Helpers/NumericHelper.h"

namespace consolidator::dsp {

class GainFilter final : public IEqFilter {
public:
    explicit GainFilter(GainFilterSettings settings)
        : settings(settings) {}

    double ProcessSample(double input) override {
        return input * helpers::NumericHelper::DecibelsToMagnitude(settings.gainDb);
    }

    double GetMagnitudeDb(double) const override {
        return settings.gainDb;
    }

    double GetPhaseRadians(double) const override {
        return 0.0;
    }

    void Reset() override {}

private:
    GainFilterSettings settings;
};

} // namespace consolidator::dsp
