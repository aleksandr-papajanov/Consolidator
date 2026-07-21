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

class OfflineFitEvaluator final {
public:
    std::optional<double> Evaluate(
        const consolidator::domain::DspSnapshot& snapshot,
        const FitAudioBuffer& audio
    ) {
        if (audio.Size() == 0) return std::nullopt;

        consolidator::dspcore::DeviceDspRuntime runtime;
        runtime.SetSnapshot(snapshot);
        auto chain = runtime.BuildStereo(audio.SampleRate());
        auto frameBuffer = std::make_unique<AnalyzerFrameBuffer>();
        auto curves = std::make_unique<AnalyzerCurveBatch>();
        auto spectrumEngine = std::make_unique<AnalyzerSpectrumEngine>();
        auto featurePipeline = std::make_unique<AnalyzerFeaturePipeline>();
        auto spectra = std::make_unique<AnalyzerSpectrumResult>();
        spectrumEngine->SetSampleRate(audio.SampleRate());
        featurePipeline->SetSampleRate(audio.SampleRate());

        const auto warmupSamples = static_cast<std::size_t>(
            audio.SampleRate() * consolidator::settings::FitOptions::WarmupSeconds);
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
            spectrumEngine->AnalyzeInto(*frameBuffer, *curves, *spectra);
            result = featurePipeline->Process(*frameBuffer, *spectra);
            frameBuffer->Reset();
        }
        return result ? std::optional{ AnalysisMetricSnapshot::FromFrame(*result).Loss() }
                      : std::nullopt;
    }
};
