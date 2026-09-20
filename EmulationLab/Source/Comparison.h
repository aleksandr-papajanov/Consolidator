#pragma once

#include "Dataset.h"
#include "ExperimentManifest.h"

#include <filesystem>
#include <string>
#include <vector>

namespace emulation_lab
{

struct ProbeComparison
{
    std::string probeId;
    std::size_t matchedObservations{};
    std::size_t matchedFeatures{};
    double normalizedRmse{};
};

struct ComparisonReport
{
    std::string experimentName;
    std::string configurationSignature;
    std::size_t matchedObservations{};
    std::size_t unmatchedReferenceObservations{};
    double weightedLoss{};
    std::vector<ProbeComparison> probes;
};

[[nodiscard]] double FeatureGroupWeight(
    const std::string& group,
    const ObjectiveWeights& objective);

[[nodiscard]] ComparisonReport CompareDatasets(
    const MeasurementDataset& reference,
    const MeasurementDataset& candidate,
    const ObjectiveWeights& objective);

void SaveComparisonReport(
    const ComparisonReport& report,
    const std::filesystem::path& path);

[[nodiscard]] bool RunComparisonSelfTest();

} // namespace emulation_lab
