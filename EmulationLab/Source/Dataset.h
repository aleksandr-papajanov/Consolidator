#pragma once

#include "Probe.h"

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

namespace emulation_lab
{

inline constexpr int kDatasetSchemaVersion = 3;

struct DatasetObservation
{
    std::string observationId;
    std::string conditionId;
    std::string sequenceId;
    std::size_t sequenceIndex{};
    std::string probeId;
    std::string probeVersion;
    double sampleRate{};
    double inputLevelDb{};
    double frequency{};
    std::vector<ProbeFeature> features;
};

struct MeasurementDataset
{
    int schemaVersion{ kDatasetSchemaVersion };
    std::string experimentName;
    std::string configurationSignature;
    std::string side;
    std::string runId;
    std::string environmentFile;
    std::vector<DatasetObservation> observations;
};

void SaveDataset(const MeasurementDataset& dataset, const std::filesystem::path& path);
[[nodiscard]] MeasurementDataset LoadDataset(const std::filesystem::path& path);
[[nodiscard]] std::pair<MeasurementDataset, MeasurementDataset> SplitDataset(
    const MeasurementDataset& source,
    double validationFraction);

} // namespace emulation_lab
