#pragma once

#include "c74_min.h"
#include "DSP/Curve/Curve.h"
#include "Settings/AnalysisOptions.h"

#include <stdexcept>
#include <utility>
#include <vector>

class ApproximatorCurveStore {
public:
    void SetTarget(const c74::min::atoms& values) {
        hasTarget = Assign(differenceCurve, values);
    }

    void SetCurrentEq(consolidator::dsp::Curve curve) {
        currentEqCurve = std::move(curve);
        hasCurrentEq = true;
    }

    void ClearTarget() {
        differenceCurve = consolidator::dsp::Curve{};
        hasTarget = false;
    }

    bool HasTarget() const {
        return hasTarget;
    }

    bool HasCurrentEq() const {
        return hasCurrentEq;
    }

    bool HasCompatibleCurves() const {
        return hasTarget && hasCurrentEq &&
            differenceCurve.Settings() == currentEqCurve.Settings();
    }

    consolidator::dsp::Curve CombinedCurve() const {
        if (!HasCompatibleCurves()) throw std::runtime_error("missing_or_incompatible_curve_input");
        return differenceCurve + currentEqCurve;
    }

private:
    static bool Assign(consolidator::dsp::Curve& curve, const c74::min::atoms& values) {
        if (values.size() != consolidator::settings::AnalysisOptions::DefaultCurvePointCount) {
            curve = consolidator::dsp::Curve{};
            return false;
        }
        std::vector<double> curveValues;
        curveValues.reserve(values.size());
        for (std::size_t index = 0; index < values.size(); ++index) {
            curveValues.push_back(static_cast<double>(values[index]));
        }
        curve = consolidator::dsp::Curve::FromValues(std::move(curveValues));
        return true;
    }

    consolidator::dsp::Curve differenceCurve;
    consolidator::dsp::Curve currentEqCurve;
    bool hasTarget = false;
    bool hasCurrentEq = false;
};
