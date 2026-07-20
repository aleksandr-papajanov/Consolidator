#pragma once

#include "AnalyzerCurveFrame.h"
#include "DSP/Curve/Curve.h"
#include "Settings/AnalysisOptions.h"
#include "Settings/SpectrumOptions.h"

#include <algorithm>

class AnalyzerCurveBatch {
public:
    int Prepare(int binsOut = static_cast<int>(consolidator::settings::AnalysisOptions::DefaultCurvePointCount)) {
        if (lastPendingCount != binsOut) {
            ResetCurves(binsOut);
            smoothingInitialized = false;
            differenceSmoothingInitialized = false;
            lastPendingCount = binsOut;
        }

        const int previousPendingCount = pendingCount;
        pendingCount = binsOut;

        return previousPendingCount;
    }

    void StoreBin(
        int outputIndex,
        int previousPendingCount,
        double rawCurrentDb,
        double rawReferenceDb,
        double smoothing = consolidator::settings::AnalysisOptions::DefaultSpectrumSmoothing,
        double lowFrequencyAmount = consolidator::settings::AnalysisOptions::DefaultLowFrequencySmoothing,
        double spectrumCalibrationDb = consolidator::settings::SpectrumOptions::DefaultSpectrumCalibrationDb,
        double spectrumTiltDb = consolidator::settings::SpectrumOptions::DefaultSpectrumTiltDb
    ) {
        const double tiltWeight = consolidator::dsp::Curve::HighFrequencyWeight(
            static_cast<std::size_t>(outputIndex),
            static_cast<std::size_t>(pendingCount));
        const double tiltOffset = spectrumTiltDb * tiltWeight;
        const double currentDb = std::clamp(rawCurrentDb + spectrumCalibrationDb + tiltOffset,
            consolidator::settings::SpectrumOptions::MinimumSpectrumDb,
            consolidator::settings::SpectrumOptions::MaximumSpectrumDb);
        const double referenceDb = std::clamp(rawReferenceDb + spectrumCalibrationDb + tiltOffset,
            consolidator::settings::SpectrumOptions::MinimumSpectrumDb,
            consolidator::settings::SpectrumOptions::MaximumSpectrumDb);
        const double differenceDb = std::clamp(rawReferenceDb - rawCurrentDb,
            consolidator::settings::SpectrumOptions::MinimumDifferenceDb,
            consolidator::settings::SpectrumOptions::MaximumDifferenceDb);
        const double adaptiveSmoothing = consolidator::dsp::Curve::FrequencyDependentSmoothing(
            static_cast<std::size_t>(outputIndex),
            static_cast<std::size_t>(pendingCount),
            smoothing,
            lowFrequencyAmount);

        if (!smoothingInitialized || outputIndex >= previousPendingCount) {
            smoothedCurrent.SetValue(outputIndex, currentDb);
            smoothedReference.SetValue(outputIndex, referenceDb);
        }
        else {
            smoothedCurrent.SmoothValue(outputIndex, currentDb, adaptiveSmoothing);
            smoothedReference.SmoothValue(outputIndex, referenceDb, adaptiveSmoothing);
        }

        if (!differenceSmoothingInitialized || outputIndex >= previousPendingCount) {
            smoothedDifference.SetValue(outputIndex, differenceDb);
        }
        else {
            smoothedDifference.SmoothValue(outputIndex, differenceDb, adaptiveSmoothing);
        }

        pendingCurrent.SetValue(outputIndex, smoothedCurrent.Values().at(outputIndex));
        pendingReference.SetValue(outputIndex, smoothedReference.Values().at(outputIndex));
        pendingDifference.SetValue(outputIndex, smoothedDifference.Values().at(outputIndex));
    }

    void FinalizeFrame() {
        smoothingInitialized = true;
        differenceSmoothingInitialized = true;
    }

    void ResetDifference() {
        differenceSmoothingInitialized = false;
    }

    void WriteFrame(AnalyzerCurveFrame& frame, std::uint64_t differenceGeneration) const {
        frame.Assign(
            pendingCurrent,
            pendingReference,
            pendingDifference,
            pendingCount,
            differenceGeneration);
    }

private:
    static consolidator::dsp::Curve MakeCurve(
        int pointCount = static_cast<int>(consolidator::settings::AnalysisOptions::DefaultCurvePointCount)
    ) {
        auto settings = consolidator::dsp::CurveSettings{};
        settings.pointCount = static_cast<std::size_t>(pointCount);
        return consolidator::dsp::Curve{ settings };
    }

    void ResetCurves(int pointCount) {
        pendingCurrent = MakeCurve(pointCount);
        pendingReference = MakeCurve(pointCount);
        pendingDifference = MakeCurve(pointCount);
        smoothedCurrent = MakeCurve(pointCount);
        smoothedReference = MakeCurve(pointCount);
        smoothedDifference = MakeCurve(pointCount);
    }

    consolidator::dsp::Curve pendingCurrent;
    consolidator::dsp::Curve pendingReference;
    consolidator::dsp::Curve pendingDifference;
    consolidator::dsp::Curve smoothedCurrent;
    consolidator::dsp::Curve smoothedReference;
    consolidator::dsp::Curve smoothedDifference;

    int pendingCount = 0;
    int lastPendingCount = 0;
    bool smoothingInitialized = false;
    bool differenceSmoothingInitialized = false;
};
