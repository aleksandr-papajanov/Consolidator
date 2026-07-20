#pragma once

#include "c74_min.h"
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
        bool sendDifference,
        const consolidator::dsp::Curve& selectedPrefixCurve
    ) const {
        c74::min::atoms currentAtoms;
        c74::min::atoms referenceAtoms;
        c74::min::atoms differenceAtoms;
        const auto& currentValues = current.Values();
        const auto& referenceValues = reference.Values();
        const auto& differenceValues = difference.Values();
        const auto& eqCurve = selectedPrefixCurve.Values();

        for (int index = 0; index < pointCount; ++index) {
            const auto offset = index < static_cast<int>(eqCurve.size())
                ? eqCurve[static_cast<std::size_t>(index)]
                : 0.0;
            currentAtoms.push_back(currentValues[static_cast<std::size_t>(index)] + offset);
            referenceAtoms.push_back(referenceValues[static_cast<std::size_t>(index)]);
            differenceAtoms.push_back(differenceValues[static_cast<std::size_t>(index)] - offset);
        }

        currentOut.send(currentAtoms);
        referenceOut.send(referenceAtoms);
        if (sendDifference) differenceOut.send(differenceAtoms);
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
};
