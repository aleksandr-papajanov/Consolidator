#pragma once

#include "c74_min.h"
#include "Analysis/AnalysisFeatureFrame.h"
#include "Audio/GainLevelMetrics.h"
#include "DSP/Curve/Curve.h"
#include "Settings/AnalysisOptions.h"

#include <cstdint>

class AnalyzerCurveFrame final {
public:
    AnalyzerCurveFrame()
        : current(MakeCurve()), reference(MakeCurve()), difference(MakeCurve()) {}

    void Assign(
        const consolidator::dsp::Curve& nextCurrent,
        const consolidator::dsp::Curve& nextReference,
        const consolidator::dsp::Curve& nextDifference,
        int nextPointCount,
        std::uint64_t nextDifferenceGeneration
    ) {
        current = nextCurrent;
        reference = nextReference;
        difference = nextDifference;
        pointCount = nextPointCount;
        differenceGeneration = nextDifferenceGeneration;
    }

    void Send(
        c74::min::outlet<>& currentOut,
        c74::min::outlet<>& referenceOut,
        c74::min::outlet<>& differenceOut,
        const consolidator::dsp::Curve& banksThroughSelectedCurve,
        c74::min::outlet<>& analysisOut,
        c74::min::outlet<>& levelsOut
    ) const {
        c74::min::atoms currentAtoms;
        c74::min::atoms referenceAtoms;
        const auto& differenceValues = difference.Values();
        c74::min::atoms fitCurveAtoms;

        for (int index = 0; index < pointCount; ++index) {
            currentAtoms.push_back(current.Values().at(static_cast<std::size_t>(index)));
            referenceAtoms.push_back(reference.Values().at(static_cast<std::size_t>(index)));
            fitCurveAtoms.push_back(
                differenceValues[static_cast<std::size_t>(index)]
                - banksThroughSelectedCurve.Values().at(static_cast<std::size_t>(index)));
        }

        currentOut.send(currentAtoms);
        referenceOut.send(referenceAtoms);
        fitCurveAtoms.insert(fitCurveAtoms.begin(), "fit_curve");
        differenceOut.send(fitCurveAtoms);
        features.Send(analysisOut);
        levelsOut.send(
            "gain_levels",
            gainLevels.inputPreDb,
            gainLevels.inputPostDb,
            gainLevels.outputPreDb,
            gainLevels.outputPostDb,
            gainLevels.referenceDb);
    }

    void SetFeatures(AnalysisFeatureFrame value) {
        features = std::move(value);
    }

    void SetGainLevels(consolidator::audio::GainLevelMetrics value) {
        gainLevels = value;
    }

    std::uint64_t DifferenceGeneration() const noexcept {
        return differenceGeneration;
    }

private:
    static consolidator::dsp::Curve MakeCurve() {
        auto settings = consolidator::dsp::CurveSettings{};
        settings.pointCount = consolidator::settings::AnalysisOptions::DefaultCurvePointCount;
        return consolidator::dsp::Curve{ settings };
    }

    consolidator::dsp::Curve current;
    consolidator::dsp::Curve reference;
    consolidator::dsp::Curve difference;
    int pointCount = 0;
    std::uint64_t differenceGeneration = 0;
    AnalysisFeatureFrame features;
    consolidator::audio::GainLevelMetrics gainLevels;
};
