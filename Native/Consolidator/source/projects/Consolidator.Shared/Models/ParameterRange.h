#pragma once

#include "../Helpers/NumericHelper.h"

namespace consolidator::models {

enum class ParameterScale {
    Linear,
    Logarithmic
};

struct ParameterRange {
    double minimum = 0.0;
    double maximum = 1.0;
    ParameterScale scale = ParameterScale::Linear;

    double Normalize(double value) const {
        if (scale == ParameterScale::Logarithmic) {
            return helpers::NumericHelper::LogarithmicToUnit(value, minimum, maximum);
        }
        return helpers::NumericHelper::LinearToUnit(value, minimum, maximum);
    }

    double Denormalize(double value) const {
        if (scale == ParameterScale::Logarithmic) {
            return helpers::NumericHelper::UnitToLogarithmic(value, minimum, maximum);
        }
        return helpers::NumericHelper::UnitToLinear(value, minimum, maximum);
    }
};

} // namespace consolidator::models
