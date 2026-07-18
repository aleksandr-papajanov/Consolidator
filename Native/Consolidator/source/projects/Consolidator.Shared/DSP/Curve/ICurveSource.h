#pragma once

namespace consolidator::dsp {

class ICurveSource {
public:
    virtual ~ICurveSource() = default;

    virtual double EvaluateCurve(double input) const = 0;
};

} // namespace consolidator::dsp
