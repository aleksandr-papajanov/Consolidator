#pragma once

#include "BiquadHighShelfFilter.h"
#include "BiquadLowShelfFilter.h"
#include "FilterSettings.h"
#include "../IEqFilter.h"

namespace consolidator::dsp {

class TiltFilter final : public IEqFilter {
public:
    explicit TiltFilter(TiltFilterSettings settings)
        : settings(settings),
          lowShelf({ settings.pivotHz, settings.q, -settings.gainDb, settings.sampleRate }),
          highShelf({ settings.pivotHz, settings.q, settings.gainDb, settings.sampleRate }) {}

    double ProcessSample(double input) override {
        return highShelf.ProcessSample(lowShelf.ProcessSample(input));
    }

    double GetMagnitudeDb(double frequencyHz) const override {
        return lowShelf.GetMagnitudeDb(frequencyHz) + highShelf.GetMagnitudeDb(frequencyHz);
    }

    double GetPhaseRadians(double frequencyHz) const override {
        return lowShelf.GetPhaseRadians(frequencyHz) + highShelf.GetPhaseRadians(frequencyHz);
    }

    void Reset() override {
        lowShelf.Reset();
        highShelf.Reset();
    }

private:
    TiltFilterSettings settings;
    BiquadLowShelfFilter lowShelf;
    BiquadHighShelfFilter highShelf;
};

} // namespace consolidator::dsp
