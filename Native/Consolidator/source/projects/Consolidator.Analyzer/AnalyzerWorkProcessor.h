#pragma once

#include "AnalyzerCurveBatch.h"
#include "AnalyzerCurveFrame.h"
#include "AnalyzerFrameBuffer.h"
#include "AnalyzerSpectrumEngine.h"
#include "Analysis/AnalyzerFeaturePipeline.h"
#include "Audio/GainLevelMetrics.h"
#include "Workflows/LatestWorkflowExecutor.h"

#include <cstdint>

struct AnalyzerWorkTask final {
    AnalyzerFrameBuffer visualFrame;
    AnalyzerFrameBuffer fitFrame;
    consolidator::audio::GainLevelMetrics gainLevels;
    double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz;
    std::uint64_t differenceGeneration = 0;
    bool visualReady = false;
    bool fitReady = false;
    bool sendSpectrum = false;
    bool sendAnalysis = false;
    bool resetDifference = false;
    bool collecting = false;
};

struct AnalyzerWorkResult final {
    AnalyzerCurveFrame frame;
    bool publish = false;
};

class AnalyzerWorkProcessor final {
public:
    AnalyzerWorkResult Process(
        const AnalyzerWorkTask& task,
        const consolidator::workflows::WorkflowCancellation& cancellation
    ) {
        AnalyzerWorkResult result;
        if (cancellation.IsRequested()) return result;

        if (task.sampleRate != sampleRate) {
            sampleRate = task.sampleRate;
            spectrumEngine.SetSampleRate(sampleRate);
            featurePipeline.SetSampleRate(sampleRate);
        }
        if (task.resetDifference) {
            curves.ResetDifference();
            fitCurves.ResetDifference();
        }

        const auto currentActive = task.visualReady && !task.visualFrame.IsCurrentSilent();
        const auto referenceActive = task.visualReady && !task.visualFrame.IsReferenceSilent();
        const auto visualActive = currentActive || referenceActive;
        const auto fitReferenceActive = task.fitReady && !task.fitFrame.IsReferenceSilent();
        const auto fitActive = task.fitReady && !task.fitFrame.IsCurrentSilent() &&
            !task.fitFrame.IsReferenceSilent();

        if (!visualActive && !fitActive) {
            if (!audioActive) return result;
            audioActive = false;
            result.frame.MarkSilent();
            result.publish = true;
            return result;
        }

        audioActive = true;
        if (visualActive) {
            spectrumEngine.AnalyzeInto(
                task.visualFrame,
                curves,
                visualSpectra,
                task.sendSpectrum,
                false);
        }
        if (cancellation.IsRequested()) return {};

        if (fitActive) {
            if (visualActive) {
                spectrumEngine.AnalyzeCurrentWithReferenceInto(
                    task.fitFrame,
                    visualSpectra.reference,
                    fitCurves,
                    fitSpectra,
                    true);
            }
            else {
                spectrumEngine.Analyze(task.fitFrame, fitCurves, true, true);
            }
        }
        if (cancellation.IsRequested()) return {};

        if (task.sendSpectrum && task.visualReady && fitActive) {
            curves.WriteFrame(
                result.frame,
                fitCurves,
                task.differenceGeneration,
                referenceActive,
                true);
        }
        else if (task.sendSpectrum && task.visualReady) {
            curves.WriteFrame(
                result.frame,
                task.differenceGeneration,
                referenceActive,
                false);
        }
        else if (fitActive) {
            fitCurves.WriteFrame(
                result.frame,
                task.differenceGeneration,
                fitReferenceActive,
                true);
        }

        if (visualActive && task.sendAnalysis) {
            result.frame.SetFeatures(featurePipeline.Process(task.visualFrame, visualSpectra));
        }
        auto levels = task.gainLevels;
        levels.referenceDb = task.fitFrame.ReferenceLevelDb();
        result.frame.SetGainLevels(levels);
        result.publish = true;
        return result;
    }

private:
    double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz;
    AnalyzerSpectrumEngine spectrumEngine;
    AnalyzerFeaturePipeline featurePipeline;
    AnalyzerCurveBatch curves;
    AnalyzerCurveBatch fitCurves;
    AnalyzerSpectrumResult visualSpectra;
    AnalyzerSpectrumResult fitSpectra;
    bool audioActive = false;
};
