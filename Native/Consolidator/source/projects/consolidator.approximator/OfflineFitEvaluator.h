#pragma once

#include "AnalysisMetricSnapshot.h"
#include "FitAudioBuffer.h"
#include "Analysis/AnalyzerFeaturePipeline.h"
#include "AnalyzerCurveBatch.h"
#include "AnalyzerFrameBuffer.h"
#include "AnalyzerSpectrumEngine.h"
#include "DeviceDspRuntime.h"
#include "Settings/FitOptions.h"

#include <cstddef>
#include <memory>
#include <optional>
#include <vector>

class OfflineFitEvaluator final {
public:
    OfflineFitEvaluator()
        : frameBuffer(std::make_unique<AnalyzerFrameBuffer>()),
          curves(std::make_unique<AnalyzerCurveBatch>()),
          spectrumEngine(std::make_unique<AnalyzerSpectrumEngine>()),
          featurePipeline(std::make_unique<AnalyzerFeaturePipeline>()),
          spectra(std::make_unique<AnalyzerSpectrumResult>()) {}

    std::optional<double> Evaluate(
        const consolidator::domain::DspSnapshot& snapshot,
        const FitAudioBuffer& audio
    ) {
        if (audio.Size() == 0) return std::nullopt;
        if (!PrepareReference(audio)) return std::nullopt;

        consolidator::dspcore::DeviceDspRuntime runtime;
        runtime.SetSnapshot(snapshot);
        auto chain = runtime.BuildStereo(audio.SampleRate());
        frameBuffer->Reset();
        curves->Reset();
        spectrumEngine->SetSampleRate(audio.SampleRate());
        featurePipeline->Reset(audio.SampleRate());

        const auto warmupSamples = static_cast<std::size_t>(
            audio.SampleRate() * consolidator::settings::FitOptions::WarmupSeconds);
        std::size_t referenceIndex = 0;
        std::optional<AnalysisFeatureFrame> result;
        for (std::size_t index = 0; index < audio.Size(); ++index) {
            const auto processed = chain.ProcessSample({
                audio.CurrentLeft()[index], audio.CurrentRight()[index]
            });
            if (index < warmupSamples) continue;
            frameBuffer->Write({
                processed,
                { audio.ReferenceLeft()[index], audio.ReferenceRight()[index] }
            });
            if (!frameBuffer->Advance()) continue;
            if (referenceIndex >= referenceSpectra.size()) return std::nullopt;
            spectrumEngine->AnalyzeCurrentWithReferenceInto(
                *frameBuffer,
                referenceSpectra[referenceIndex++],
                *curves,
                *spectra);
            result = featurePipeline->Process(*frameBuffer, *spectra);
            frameBuffer->Reset();
        }
        return result ? std::optional{ AnalysisMetricSnapshot::FromFrame(*result).Loss() }
                      : std::nullopt;
    }

private:
    bool PrepareReference(const FitAudioBuffer& audio) {
        if (cachedAudio == &audio) return !referenceSpectra.empty();

        referenceSpectra.clear();
        referenceFrame->Reset();
        spectrumEngine->SetSampleRate(audio.SampleRate());
        const auto warmupSamples = static_cast<std::size_t>(
            audio.SampleRate() * consolidator::settings::FitOptions::WarmupSeconds);
        for (std::size_t index = warmupSamples; index < audio.Size(); ++index) {
            referenceFrame->Write({
                {},
                { audio.ReferenceLeft()[index], audio.ReferenceRight()[index] }
            });
            if (!referenceFrame->Advance()) continue;
            referenceSpectra.emplace_back();
            spectrumEngine->AnalyzeReferenceInto(
                *referenceFrame,
                referenceSpectra.back());
            referenceFrame->Reset();
        }
        cachedAudio = &audio;
        return !referenceSpectra.empty();
    }

    std::unique_ptr<AnalyzerFrameBuffer> frameBuffer;
    std::unique_ptr<AnalyzerFrameBuffer> referenceFrame =
        std::make_unique<AnalyzerFrameBuffer>();
    std::unique_ptr<AnalyzerCurveBatch> curves;
    std::unique_ptr<AnalyzerSpectrumEngine> spectrumEngine;
    std::unique_ptr<AnalyzerFeaturePipeline> featurePipeline;
    std::unique_ptr<AnalyzerSpectrumResult> spectra;
    std::vector<AnalyzerSignalSpectrum> referenceSpectra;
    const FitAudioBuffer* cachedAudio = nullptr;
};
