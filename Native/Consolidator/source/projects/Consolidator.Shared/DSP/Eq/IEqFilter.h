#pragma once

#include "../IDspDevice.h"
#include "../Curve/ICurveSource.h"

namespace consolidator::dsp {

class IEqFilter : public IDspDevice, public ICurveSource {
public:
    virtual double GetMagnitudeDb(double frequencyHz) const = 0;
    virtual double GetPhaseRadians(double frequencyHz) const = 0;

    double EvaluateCurve(double frequencyHz) const override {
        return GetMagnitudeDb(frequencyHz);
    }
};

} // namespace consolidator::dsp
