#pragma once

#include "../Helpers/NumericHelper.h"

#include <cmath>

namespace consolidator::audio {

struct StereoSample {
    double left = 0.0;
    double right = 0.0;

    static StereoSample FromDecibels(double leftDb, double rightDb) {
        return {
            helpers::NumericHelper::DecibelsToMagnitude(leftDb),
            helpers::NumericHelper::DecibelsToMagnitude(rightDb)
        };
    }

    double Magnitude() const {
        return 0.5 * (std::abs(left) + std::abs(right));
    }

    double MagnitudeDb() const {
        return helpers::NumericHelper::MagnitudeToDecibels(Magnitude());
    }

    StereoSample operator+(const StereoSample& other) const {
        return { left + other.left, right + other.right };
    }

    StereoSample operator-(const StereoSample& other) const {
        return { left - other.left, right - other.right };
    }

    StereoSample operator*(double scalar) const {
        return { left * scalar, right * scalar };
    }
};

} // namespace consolidator::audio
