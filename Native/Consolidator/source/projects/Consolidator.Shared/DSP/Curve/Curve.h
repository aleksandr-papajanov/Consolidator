#pragma once

#include "../../Helpers/NumericHelper.h"
#include "CurveSettings.h"

#include <cstddef>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <vector>

namespace consolidator::dsp {

class Curve {
public:
    explicit Curve(CurveSettings settings = {})
        : settings(settings), inputs(BuildInputs(settings)), values(settings.pointCount, 0.0) {}

    static Curve FromValues(std::vector<double> values, CurveSettings settings = {}) {
        Curve curve{ settings };
        curve.SetValues(std::move(values));
        return curve;
    }

    const CurveSettings& Settings() const {
        return settings;
    }

    const std::vector<double>& Inputs() const {
        return inputs;
    }

    const std::vector<double>& Values() const {
        return values;
    }

    void SetValue(std::size_t index, double value) {
        values.at(index) = value;
    }

    void AddValue(std::size_t index, double value) {
        values.at(index) += value;
    }

    void SetValues(std::vector<double> values) {
        if (values.size() != this->values.size()) {
            throw std::invalid_argument("Curve value count must match settings");
        }
        this->values = std::move(values);
    }

    void Clear(double value = 0.0) {
        std::fill(values.begin(), values.end(), value);
    }

    void SmoothValue(std::size_t index, double target, double smoothing) {
        const double amount = helpers::NumericHelper::Clamp(smoothing, 0.0, 1.0);
        values.at(index) = values.at(index) * amount + target * (1.0 - amount);
    }

    void SmoothTowards(const Curve& target, double smoothing) {
        EnsureCompatible(target);
        for (std::size_t index = 0; index < values.size(); ++index) {
            SmoothValue(index, target.values[index], smoothing);
        }
    }

    static double FrequencyDependentSmoothing(
        std::size_t index,
        std::size_t pointCount,
        double smoothing = settings::AnalysisOptions::DefaultSpectrumSmoothing,
        double lowFrequencyAmount = settings::AnalysisOptions::DefaultLowFrequencySmoothing
    ) {
        if (pointCount <= 1) {
            return smoothing;
        }

        const double normalized = static_cast<double>(index) /
            static_cast<double>(pointCount - 1);
        const double lowFrequencyWeight = std::pow(
            std::max(0.0, 1.0 - normalized),
            settings::AnalysisOptions::LowFrequencySmoothingExponent);
        constexpr double maximumSmoothing = settings::AnalysisOptions::MaximumCurveSmoothing;
        const double boosted = smoothing +
            (maximumSmoothing - smoothing) * lowFrequencyAmount * lowFrequencyWeight;

        return helpers::NumericHelper::Clamp(boosted, 0.0, maximumSmoothing);
    }

private:
    void EnsureCompatible(const Curve& other) const {
        if (settings != other.settings) {
            throw std::invalid_argument("Curve settings must match");
        }
    }

    static std::vector<double> BuildInputs(const CurveSettings& settings) {
        if (settings.pointCount == 0 || !helpers::NumericHelper::IsStrictlyIncreasingRange(
                settings.minimumInput, settings.maximumInput) ||
            (settings.scale == CurveScale::Logarithmic && settings.minimumInput <= 0.0)) {
            throw std::invalid_argument("Curve settings are invalid");
        }

        std::vector<double> inputs;
        inputs.reserve(settings.pointCount);
        const double minimum = settings.scale == CurveScale::Logarithmic
            ? std::log(settings.minimumInput) : settings.minimumInput;
        const double maximum = settings.scale == CurveScale::Logarithmic
            ? std::log(settings.maximumInput) : settings.maximumInput;
        for (std::size_t index = 0; index < settings.pointCount; ++index) {
            const double position = settings.pointCount == 1 ? 0.0 :
                static_cast<double>(index) / static_cast<double>(settings.pointCount - 1);
            const double value = minimum + position * (maximum - minimum);
            inputs.push_back(settings.scale == CurveScale::Logarithmic ? std::exp(value) : value);
        }
        return inputs;
    }

    CurveSettings settings;
    std::vector<double> inputs;
    std::vector<double> values;

    friend Curve operator+(const Curve& left, const Curve& right);
    friend Curve operator-(const Curve& left, const Curve& right);
};

inline Curve operator+(const Curve& left, const Curve& right) {
    left.EnsureCompatible(right);

    Curve result{ left.settings };
    for (std::size_t index = 0; index < result.values.size(); ++index) {
        result.values[index] = left.values[index] + right.values[index];
    }
    return result;
}

inline Curve operator-(const Curve& left, const Curve& right) {
    left.EnsureCompatible(right);

    Curve result{ left.settings };
    for (std::size_t index = 0; index < result.values.size(); ++index) {
        result.values[index] = left.values[index] - right.values[index];
    }
    return result;
}

} // namespace consolidator::dsp
