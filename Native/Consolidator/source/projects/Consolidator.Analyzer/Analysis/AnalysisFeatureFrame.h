#pragma once

#include "c74_min.h"
#include "FrequencyBands.h"

#include <array>
#include <cstddef>
#include <string_view>

class AnalysisFeatureFrame final {
public:
    static constexpr std::size_t MetricCount = 8;
    static constexpr std::size_t BandCount = FrequencyBands::Count;
    struct SignalStatistics final {
        std::array<double, MetricCount> metricMeans{};
        std::array<double, MetricCount> metricDeviations{};
        std::array<std::array<double, MetricCount>, BandCount> bandMetricMeans{};
        std::array<std::array<double, MetricCount>, BandCount> bandMetricDeviations{};
    };

    void Send(c74::min::outlet<>& outlet) const {
        SendFeatures(outlet);
    }

    SignalStatistics current;
    SignalStatistics reference;
    std::size_t windowCount = 0;
    double historySeconds = 0.0;

private:
    static constexpr std::array<std::string_view, MetricCount> MetricNames{
        "rms_db", "peak_db", "crest_db", "centroid_hz",
        "flatness", "flux", "transient_db", "spectral_similarity"
    };
    void SendFeatures(c74::min::outlet<>& outlet) const {
        c74::min::atoms output;
        output.reserve(6 + MetricCount * 6 + BandCount * (2 + MetricCount * 4));
        output.push_back("feature_vector");
        output.push_back(static_cast<long>(windowCount));
        output.push_back(historySeconds);
        output.push_back(static_cast<long>(MetricCount));
        for (std::size_t index = 0; index < MetricCount; ++index) {
            output.push_back(std::string{ MetricNames[index] });
            output.push_back(current.metricMeans[index]);
            output.push_back(current.metricDeviations[index]);
            output.push_back(reference.metricMeans[index]);
            output.push_back(reference.metricDeviations[index]);
        }
        output.push_back(static_cast<long>(BandCount));
        output.push_back(static_cast<long>(MetricCount));
        for (const auto name : MetricNames) output.push_back(std::string{ name });
        for (std::size_t index = 0; index < BandCount; ++index) {
            output.push_back(FrequencyBands::Standard[index].minimumHz);
            output.push_back(FrequencyBands::Standard[index].maximumHz);
            for (std::size_t metric = 0; metric < MetricCount; ++metric) {
                output.push_back(current.bandMetricMeans[index][metric]);
                output.push_back(current.bandMetricDeviations[index][metric]);
                output.push_back(reference.bandMetricMeans[index][metric]);
                output.push_back(reference.bandMetricDeviations[index][metric]);
            }
        }
        outlet.send(output);
    }
};
