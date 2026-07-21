#pragma once

#include "Analysis/AnalysisFeatureFrame.h"
#include "Analysis/FrequencyBands.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

class AnalysisMetricSnapshot final {
public:
    struct Metric final {
        std::string id;
        double current = 0.0;
        double currentDeviation = 0.0;
        double reference = 0.0;
        double referenceDeviation = 0.0;
        double minimumHz = 10.0;
        double maximumHz = 20000.0;
    };

    static AnalysisMetricSnapshot FromFrame(const AnalysisFeatureFrame& frame) {
        AnalysisMetricSnapshot result;
        result.windowCount = static_cast<long>(frame.windowCount);
        result.historySeconds = frame.historySeconds;
        for (std::size_t index = 0; index < AnalysisFeatureFrame::MetricCount; ++index) {
            result.metrics.push_back({
                std::string{ AnalysisFeatureFrame::MetricNames[index] },
                frame.current.metricMeans[index],
                frame.current.metricDeviations[index],
                frame.reference.metricMeans[index],
                frame.reference.metricDeviations[index]
            });
        }
        for (std::size_t band = 0; band < AnalysisFeatureFrame::BandCount; ++band) {
            for (std::size_t metric = 0; metric < AnalysisFeatureFrame::MetricCount; ++metric) {
                result.metrics.push_back({
                    std::string{ AnalysisFeatureFrame::MetricNames[metric] },
                    frame.current.bandMetricMeans[band][metric],
                    frame.current.bandMetricDeviations[band][metric],
                    frame.reference.bandMetricMeans[band][metric],
                    frame.reference.bandMetricDeviations[band][metric],
                    FrequencyBands::Standard[band].minimumHz,
                    FrequencyBands::Standard[band].maximumHz
                });
            }
        }
        return result;
    }

    double Loss() const {
        if (metrics.empty()) return 0.0;
        double squareSum = 0.0;
        for (const auto& metric : metrics) {
            const auto difference = metric.reference - metric.current;
            const auto pooledDeviation = std::sqrt(
                (metric.currentDeviation * metric.currentDeviation +
                 metric.referenceDeviation * metric.referenceDeviation) * 0.5);
            const auto scale = std::max(NormalizationFloor(metric), pooledDeviation);
            const auto magnitude = std::abs(difference) / scale;
            const auto score = 1.0 - std::exp(-magnitude * 1.35);
            squareSum += score * score;
        }
        return std::sqrt(squareSum / static_cast<double>(metrics.size()));
    }

    long WindowCount() const { return windowCount; }

private:
    static double NormalizationFloor(const Metric& metric) {
        if (metric.id == "rms_db" || metric.id == "peak_db" || metric.id == "transient_db") return 0.75;
        if (metric.id == "crest_db") return 0.5;
        if (metric.id == "centroid_hz") return std::max(15.0, (metric.maximumHz - metric.minimumHz) * 0.015);
        if (metric.id == "spectral_similarity") return 0.08;
        return 0.015;
    }

    long windowCount = 0;
    double historySeconds = 0.0;
    std::vector<Metric> metrics;
};
