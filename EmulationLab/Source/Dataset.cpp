#include "Dataset.h"

#include <cmath>
#include <cstdint>
#include <fstream>
#include <limits>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include <nlohmann/json.hpp>

namespace emulation_lab
{

namespace
{

using Json = nlohmann::json;

const char* ComparisonName(FeatureComparison comparison) noexcept
{
    return comparison == FeatureComparison::WrappedDegrees ? "wrapped-degrees" : "linear";
}

FeatureComparison ParseComparison(const std::string& value)
{
    if (value == "linear")
    {
        return FeatureComparison::Linear;
    }
    if (value == "wrapped-degrees")
    {
        return FeatureComparison::WrappedDegrees;
    }
    throw std::runtime_error("Unknown feature comparison: " + value);
}

Json ToJson(const ProbeFeature& feature)
{
    Json result{
        { "id", feature.identifier }, { "group", feature.group },
        { "unit", feature.unit }, { "value", feature.value },
        { "scale", feature.scale }, { "comparison", ComparisonName(feature.comparison) }
    };
    if (std::isfinite(feature.floor))
    {
        result["floor"] = feature.floor;
    }
    return result;
}

ProbeFeature FeatureFromJson(const Json& value)
{
    ProbeFeature feature;
    feature.identifier = value.value("id", "");
    feature.group = value.value("group", "");
    feature.unit = value.value("unit", "");
    feature.value = value.value("value", 0.0);
    feature.scale = value.value("scale", 1.0);
    feature.floor = value.value("floor", -std::numeric_limits<double>::infinity());
    feature.comparison = ParseComparison(value.value("comparison", "linear"));
    const auto validFloor = std::isfinite(feature.floor) ||
        feature.floor == -std::numeric_limits<double>::infinity();
    if (feature.identifier.empty() || feature.group.empty() ||
        !std::isfinite(feature.value) || !std::isfinite(feature.scale) || feature.scale <= 0.0)
    {
        throw std::runtime_error("Dataset contains an invalid feature.");
    }
    if (!validFloor)
    {
        throw std::runtime_error("Dataset contains an invalid feature floor.");
    }
    return feature;
}

Json ToJson(const DatasetObservation& observation)
{
    Json features = Json::array();
    for (const auto& feature : observation.features)
    {
        features.push_back(ToJson(feature));
    }
    return {
        { "id", observation.observationId }, { "condition", observation.conditionId },
        { "sequence", observation.sequenceId },
        { "sequenceIndex", observation.sequenceIndex }, { "probe", observation.probeId },
        { "probeVersion", observation.probeVersion }, { "sampleRate", observation.sampleRate },
        { "inputLevelDb", observation.inputLevelDb }, { "frequency", observation.frequency },
        { "features", std::move(features) }
    };
}

DatasetObservation ObservationFromJson(const Json& value)
{
    DatasetObservation observation;
    observation.observationId = value.value("id", "");
    observation.conditionId = value.value("condition", "");
    observation.sequenceId = value.value("sequence", "");
    observation.sequenceIndex = value.value("sequenceIndex", std::size_t{});
    observation.probeId = value.value("probe", "");
    observation.probeVersion = value.value("probeVersion", "");
    observation.sampleRate = value.value("sampleRate", 0.0);
    observation.inputLevelDb = value.value("inputLevelDb", 0.0);
    observation.frequency = value.value("frequency", 0.0);
    for (const auto& feature : value.value("features", Json::array()))
    {
        observation.features.push_back(FeatureFromJson(feature));
    }
    std::unordered_set<std::string> featureIds;
    for (const auto& feature : observation.features)
    {
        if (!featureIds.insert(feature.identifier).second)
        {
            throw std::runtime_error("Dataset observation repeats a feature identifier.");
        }
    }
    if (observation.observationId.empty() || observation.conditionId.empty() ||
        observation.sequenceId.empty() ||
        observation.probeId.empty() || observation.probeVersion.empty() ||
        !std::isfinite(observation.sampleRate) || observation.sampleRate <= 0.0 ||
        !std::isfinite(observation.inputLevelDb) ||
        !std::isfinite(observation.frequency) || observation.frequency < 0.0 ||
        observation.features.empty())
    {
        throw std::runtime_error("Dataset contains an invalid observation.");
    }
    return observation;
}

} // namespace

void SaveDataset(const MeasurementDataset& dataset, const std::filesystem::path& path)
{
    const auto parent = path.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }
    Json observations = Json::array();
    for (const auto& observation : dataset.observations)
    {
        observations.push_back(ToJson(observation));
    }
    const Json document{
        { "schemaVersion", kDatasetSchemaVersion }, { "experiment", dataset.experimentName },
        { "configurationSignature", dataset.configurationSignature },
        { "side", dataset.side }, { "runId", dataset.runId },
        { "environment", dataset.environmentFile }, { "observations", std::move(observations) }
    };
    std::ofstream output(path);
    if (!output)
    {
        throw std::runtime_error("Could not write dataset: " + path.string());
    }
    output << document.dump(2) << '\n';
}

MeasurementDataset LoadDataset(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("Could not open dataset: " + path.string());
    }
    Json document;
    try
    {
        input >> document;
    }
    catch (const Json::exception& exception)
    {
        throw std::runtime_error("Could not parse dataset: " + std::string(exception.what()));
    }
    const auto schemaVersion = document.value("schemaVersion", 0);
    if (schemaVersion != kDatasetSchemaVersion)
    {
        throw std::runtime_error("Unsupported dataset schema version: " + std::to_string(schemaVersion));
    }
    MeasurementDataset dataset;
    dataset.schemaVersion = schemaVersion;
    dataset.experimentName = document.value("experiment", "");
    dataset.configurationSignature = document.value("configurationSignature", "");
    dataset.side = document.value("side", "");
    dataset.runId = document.value("runId", "");
    dataset.environmentFile = document.value("environment", "");
    std::unordered_set<std::string> observationIds;
    for (const auto& value : document.value("observations", Json::array()))
    {
        auto observation = ObservationFromJson(value);
        if (!observationIds.insert(observation.observationId).second)
        {
            throw std::runtime_error("Dataset repeats an observation identifier.");
        }
        dataset.observations.push_back(std::move(observation));
    }
    if (dataset.experimentName.empty() || dataset.configurationSignature.empty() ||
        dataset.side.empty() || dataset.runId.empty())
    {
        throw std::runtime_error("Dataset identity is incomplete.");
    }
    return dataset;
}

std::pair<MeasurementDataset, MeasurementDataset> SplitDataset(
    const MeasurementDataset& source,
    double validationFraction)
{
    if (!std::isfinite(validationFraction) || validationFraction <= 0.0 ||
        validationFraction >= 1.0)
    {
        throw std::invalid_argument("Validation fraction must be between zero and one.");
    }
    auto training = source;
    auto validation = source;
    training.observations.clear();
    validation.observations.clear();
    for (const auto& observation : source.observations)
    {
        std::uint64_t hash = 1469598103934665603ULL;
        for (const auto value : observation.sequenceId)
        {
            hash ^= static_cast<unsigned char>(value);
            hash *= 1099511628211ULL;
        }
        const auto unit = static_cast<double>(hash % 1000000ULL) / 1000000.0;
        (unit < validationFraction ? validation : training).observations.push_back(observation);
    }
    if (training.observations.empty() || validation.observations.empty())
    {
        throw std::runtime_error("Dataset split produced an empty partition.");
    }
    training.runId += "-training";
    validation.runId += "-validation";
    return { std::move(training), std::move(validation) };
}

bool RunDatasetSelfTest()
{
    const MeasurementDataset dataset{
        kDatasetSchemaVersion, "example", "configuration-001", "reference", "run-001",
        "run-001.environment.json",
        { { "tone-44100-100--12", "default", "stepped-sine-44100", 0, "stepped-sine", "2",
            44100.0, -12.0, 100.0, { { "gain.db", "gainDb", "dB", 0.5, 1.0 } } } }
    };
    const auto path = std::filesystem::temp_directory_path() /
        "emulation-lab-dataset-v3-self-test.json";
    SaveDataset(dataset, path);
    const auto loaded = LoadDataset(path);
    std::filesystem::remove(path);
    auto splitSource = loaded;
    for (auto index = 1; index < 100; ++index)
    {
        auto observation = splitSource.observations.front();
        observation.observationId = "observation-" + std::to_string(index);
        observation.sequenceId = "sequence-" + std::to_string(index);
        splitSource.observations.push_back(std::move(observation));
    }
    const auto split = SplitDataset(splitSource, 0.5);
    return loaded.schemaVersion == kDatasetSchemaVersion &&
        loaded.experimentName == dataset.experimentName && loaded.observations.size() == 1 &&
        loaded.observations.front().features.front().identifier == "gain.db" &&
        split.first.observations.size() + split.second.observations.size() == 100;
}

} // namespace emulation_lab
