#pragma once

#include "Curve.h"
#include "ICurveSource.h"
#include <cstddef>

namespace consolidator::dsp {

class CurveRenderer {
public:
    Curve Build(const ICurveSource& source) const {
        return Build(source, {});
    }

    Curve Build(const ICurveSource& source, CurveSettings settings) const {
        Curve curve{ settings };
        const auto& inputs = curve.Inputs();
        for (std::size_t index = 0; index < inputs.size(); ++index) {
            curve.SetValue(index, source.EvaluateCurve(inputs[index]));
        }
        return curve;
    }
};

} // namespace consolidator::dsp
