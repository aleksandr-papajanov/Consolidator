#pragma once

#include "AnalysisFeatureFrame.h"
#include "FeatureExtractors.h"
#include "RollingStatistics.h"
#include "SignalAnalysisWindow.h"
#include "AnalyzerFrameBuffer.h"
#include "AnalyzerSpectrumResult.h"
#include "DSP/Curve/Curve.h"
#include "Settings/AnalysisOptions.h"
#include "Settings/AudioOptions.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <memory>
#include <span>
#include <utility>
#include <vector>

class SignalFeaturePipeline final {
public:
    SignalFeaturePipeline() {
        Register(std::make_unique<RmsFeature>());
        Register(std::make_unique<PeakFeature>());
        Register(std::make_unique<CrestFeature>());
        Register(std::make_unique<SpectralCentroidFeature>());
        Register(std::make_unique<SpectralFlatnessFeature>());
        Register(std::make_unique<SpectralFluxFeature>());
        Register(std::make_unique<TransientEnergyFeature>());
        for (const auto band : FrequencyBands::Standard) {
            RegisterBand(std::make_unique<BandRmsFeature>(band));
            RegisterBand(std::make_unique<BandPeakFeature>(band));
            RegisterBand(std::make_unique<BandCrestFeature>(band));
            RegisterBand(std::make_unique<BandCentroidFeature>(band));
            RegisterBand(std::make_unique<BandFlatnessFeature>(band));
            RegisterBand(std::make_unique<BandFluxFeature>(band));
            RegisterBand(std::make_unique<BandTransientFeature>(band));
        }
        ConfigureHistory(consolidator::settings::AudioOptions::DefaultSampleRateHz);
    }

    void SetSampleRate(double value) {
        ConfigureHistory(value);
    }

    AnalysisFeatureFrame::SignalStatistics Process(
        const SignalAnalysisWindow& window
    ) {
        for (auto& entry : entries) entry.statistics.Add(entry.extractor->Extract(window));
        for (auto& entry : bandEntries) entry.statistics.Add(entry.extractor->Extract(window));
        ++windowCount;
        return Snapshot();
    }

    std::size_t WindowCount() const { return windowCount; }
    double HistorySeconds() const { return historySeconds; }
    std::size_t HistoryCapacity() const { return historyCapacity; }

private:
    struct Entry final {
        std::unique_ptr<IFeatureExtractor> extractor;
        RollingStatistics statistics;
    };

    void Register(std::unique_ptr<IFeatureExtractor> extractor) {
        entries.push_back({ std::move(extractor), {} });
    }

    void RegisterBand(std::unique_ptr<IFeatureExtractor> extractor) {
        bandEntries.push_back({ std::move(extractor), {} });
    }

    void ConfigureHistory(double sampleRate) {
        constexpr double targetHistorySeconds = 4.0;
        const auto samplesPerWindow = static_cast<double>(
            consolidator::settings::AnalysisOptions::DefaultFftSize);
        const auto capacity = static_cast<std::size_t>(std::max(
            1.0, std::round(targetHistorySeconds * sampleRate / samplesPerWindow)));
        historySeconds = static_cast<double>(capacity) * samplesPerWindow / sampleRate;
        historyCapacity = capacity;
        windowCount = 0;
        for (auto& entry : entries) {
            entry.statistics.SetCapacity(capacity);
            entry.extractor->Reset();
        }
        for (auto& entry : bandEntries) {
            entry.statistics.SetCapacity(capacity);
            entry.extractor->Reset();
        }
    }

    AnalysisFeatureFrame::SignalStatistics Snapshot() const {
        AnalysisFeatureFrame::SignalStatistics result;
        for (std::size_t index = 0; index < entries.size(); ++index) {
            result.metricMeans[index] = entries[index].statistics.Mean();
            result.metricDeviations[index] = entries[index].statistics.StandardDeviation();
        }
        for (std::size_t band = 0; band < AnalysisFeatureFrame::BandCount; ++band) {
            for (std::size_t metric = 0; metric < entries.size(); ++metric) {
                const auto index = band * entries.size() + metric;
                result.bandMetricMeans[band][metric] = bandEntries[index].statistics.Mean();
                result.bandMetricDeviations[band][metric] = bandEntries[index].statistics.StandardDeviation();
            }
        }
        return result;
    }

    std::vector<Entry> entries;
    std::vector<Entry> bandEntries;
    std::size_t windowCount = 0;
    std::size_t historyCapacity = 1;
    double historySeconds = 0.0;
};

class AnalyzerFeaturePipeline final {
public:
    void SetSampleRate(double value) {
        sampleRate = value;
        current.SetSampleRate(value);
        reference.SetSampleRate(value);
        similarity.SetCapacity(current.HistoryCapacity());
        for (auto& statistics : bandSimilarity) {
            statistics.SetCapacity(current.HistoryCapacity());
        }
    }

    AnalysisFeatureFrame Process(
        const AnalyzerFrameBuffer& frame,
        const AnalyzerSpectrumResult& spectra
    ) {
        const auto sampleCount = static_cast<std::size_t>(frame.WriteIndex());
        const SignalAnalysisWindow currentWindow{
            std::span{ frame.CurrentLeft().data(), sampleCount },
            std::span{ frame.CurrentRight().data(), sampleCount },
            std::span{ spectra.current.magnitudes.data(), spectra.current.pointCount },
            std::span{ spectra.current.leftPowers.data(), spectra.current.pointCount },
            std::span{ spectra.current.rightPowers.data(), spectra.current.pointCount },
            std::span{ spectra.current.crossPowers.data(), spectra.current.pointCount },
            sampleRate
        };
        const SignalAnalysisWindow referenceWindow{
            std::span{ frame.ReferenceLeft().data(), sampleCount },
            std::span{ frame.ReferenceRight().data(), sampleCount },
            std::span{ spectra.reference.magnitudes.data(), spectra.reference.pointCount },
            std::span{ spectra.reference.leftPowers.data(), spectra.reference.pointCount },
            std::span{ spectra.reference.rightPowers.data(), spectra.reference.pointCount },
            std::span{ spectra.reference.crossPowers.data(), spectra.reference.pointCount },
            sampleRate
        };

        AnalysisFeatureFrame result;
        result.current = current.Process(currentWindow);
        result.reference = reference.Process(referenceWindow);
        similarity.Add(SpectralSimilarityFeature::Compare(currentWindow, referenceWindow));
        result.current.metricMeans[SimilarityMetricIndex] = similarity.Mean();
        result.current.metricDeviations[SimilarityMetricIndex] = similarity.StandardDeviation();
        result.reference.metricMeans[SimilarityMetricIndex] = 1.0;
        for (std::size_t band = 0; band < FrequencyBands::Count; ++band) {
            bandSimilarity[band].Add(SpectralSimilarityFeature::CompareBand(
                currentWindow, referenceWindow, FrequencyBands::Standard[band]));
            result.current.bandMetricMeans[band][SimilarityMetricIndex] = bandSimilarity[band].Mean();
            result.current.bandMetricDeviations[band][SimilarityMetricIndex] =
                bandSimilarity[band].StandardDeviation();
            result.reference.bandMetricMeans[band][SimilarityMetricIndex] = 1.0;
        }
        result.windowCount = std::min(current.WindowCount(), reference.WindowCount());
        result.historySeconds = std::min(current.HistorySeconds(), reference.HistorySeconds());
        return result;
    }

private:
    static constexpr std::size_t SimilarityMetricIndex = AnalysisFeatureFrame::MetricCount - 1;
    double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz;
    SignalFeaturePipeline current;
    SignalFeaturePipeline reference;
    RollingStatistics similarity;
    std::array<RollingStatistics, FrequencyBands::Count> bandSimilarity{};
};
