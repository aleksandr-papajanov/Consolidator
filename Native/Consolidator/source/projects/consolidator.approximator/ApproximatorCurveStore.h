#pragma once

#include "c74_min.h"
#include "DSP/Curve/Curve.h"
#include "Settings/AnalysisOptions.h"

#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class ApproximatorCurveStore final {
public:
    void SetFitCurve(const c74::min::atoms& values) {
        hasFitCurve = Assign(fitCurve, values);
    }

    void ClearFitCurve() {
        fitCurve = consolidator::dsp::Curve{};
        hasFitCurve = false;
    }

    bool HasFitCurve() const noexcept {
        return hasFitCurve;
    }

    bool HasCompatibleCurves() const noexcept {
        return hasFitCurve;
    }

    consolidator::dsp::Curve TargetEqCurve() const {
        if (!HasCompatibleCurves()) throw std::runtime_error("missing_fit_curve");
        return fitCurve;
    }

private:
    static bool Assign(consolidator::dsp::Curve& curve, const c74::min::atoms& values) {
        if (values.size() != consolidator::settings::AnalysisOptions::DefaultCurvePointCount + 1 ||
            static_cast<std::string>(values[0]) != "fit_curve") {
            curve = consolidator::dsp::Curve{};
            return false;
        }

        std::vector<double> magnitudes;
        magnitudes.reserve(values.size() - 1);
        for (std::size_t index = 1; index < values.size(); ++index) {
            magnitudes.push_back(static_cast<double>(values[index]));
        }
        curve = consolidator::dsp::Curve::FromValues(std::move(magnitudes));
        return true;
    }

    consolidator::dsp::Curve fitCurve;
    bool hasFitCurve = false;
};
