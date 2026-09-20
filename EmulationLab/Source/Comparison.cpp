#include "Comparison.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <limits>
#include <map>
#include <stdexcept>
#include <unordered_map>

#include <nlohmann/json.hpp>

namespace emulation_lab
{

namespace
{

double FeatureError(const ProbeFeature& reference, const ProbeFeature& candidate)
{
    if (reference.identifier != candidate.identifier || reference.group != candidate.group ||
        reference.unit != candidate.unit || reference.scale != candidate.scale ||
        reference.floor != candidate.floor ||
        reference.comparison != candidate.comparison)
    {
        throw std::runtime_error("Reference and candidate feature schemas differ.");
    }
    auto referenceValue = reference.value;
    auto candidateValue = candidate.value;
    if (std::isfinite(reference.floor))
    {
        referenceValue = std::max(referenceValue, reference.floor);
        candidateValue = std::max(candidateValue, reference.floor);
    }
    const auto difference = reference.comparison == FeatureComparison::WrappedDegrees
        ? std::remainder(candidateValue - referenceValue, 360.0)
        : candidateValue - referenceValue;
    return difference / reference.scale;
}

bool SameObservation(const DatasetObservation& left, const DatasetObservation& right)
{
    return left.observationId == right.observationId && left.probeId == right.probeId &&
        left.probeVersion == right.probeVersion && left.conditionId == right.conditionId &&
        left.sequenceId == right.sequenceId && left.sequenceIndex == right.sequenceIndex &&
        left.sampleRate == right.sampleRate && left.inputLevelDb == right.inputLevelDb &&
        left.frequency == right.frequency;
}

} // namespace

double FeatureGroupWeight(
    const std::string& group,
    const ObjectiveWeights& objective)
{
    if (group == "gainDb")
    {
        return objective.gainDb;
    }
    if (group == "phase")
    {
        return objective.phase;
    }
    if (group == "rms")
    {
        return objective.rms;
    }
    if (group == "harmonics")
    {
        return objective.harmonics;
    }
    if (group == "thd")
    {
        return objective.thd;
    }
    if (group == "peak")
    {
        return objective.peak;
    }
    if (group == "envelope")
    {
        return objective.envelope;
    }
    if (group == "intermodulation")
    {
        return objective.intermodulation;
    }
    if (group == "stereo")
    {
        return objective.stereo;
    }
    throw std::runtime_error("Unknown objective feature group: " + group);
}

ComparisonReport CompareDatasets(
    const MeasurementDataset& reference,
    const MeasurementDataset& candidate,
    const ObjectiveWeights& objective)
{
    if (reference.schemaVersion != candidate.schemaVersion ||
        reference.experimentName != candidate.experimentName ||
        reference.configurationSignature != candidate.configurationSignature ||
        reference.side != "reference" || candidate.side != "candidate")
    {
        throw std::runtime_error(
            "Datasets have incompatible roles, schemas, experiments or measurement configurations.");
    }
    std::map<std::string, ProbeComparison> metrics;
    using ProbeGroup = std::pair<std::string, std::string>;
    std::map<ProbeGroup, double> squaredErrorSums;
    std::map<ProbeGroup, std::size_t> featureCounts;
    ComparisonReport report;
    report.experimentName = reference.experimentName;
    report.configurationSignature = reference.configurationSignature;
    std::unordered_map<std::string, const DatasetObservation*> candidateIndex;
    candidateIndex.reserve(candidate.observations.size());
    for (const auto& observation : candidate.observations)
    {
        if (!candidateIndex.emplace(observation.observationId, &observation).second)
        {
            throw std::runtime_error("Candidate dataset repeats an observation identifier.");
        }
    }
    for (const auto& referenceObservation : reference.observations)
    {
        const auto candidateIterator = candidateIndex.find(referenceObservation.observationId);
        if (candidateIterator == candidateIndex.end())
        {
            ++report.unmatchedReferenceObservations;
            continue;
        }
        const auto& candidateObservation = *candidateIterator->second;
        if (!SameObservation(referenceObservation, candidateObservation))
        {
            throw std::runtime_error("Reference and candidate observation schemas differ.");
        }
        if (referenceObservation.features.size() != candidateObservation.features.size())
        {
            throw std::runtime_error("Reference and candidate feature counts differ.");
        }
        auto& metric = metrics[referenceObservation.probeId];
        metric.probeId = referenceObservation.probeId;
        for (std::size_t index = 0; index < referenceObservation.features.size(); ++index)
        {
            const auto& referenceFeature = referenceObservation.features[index];
            const auto error = FeatureError(referenceFeature, candidateObservation.features[index]);
            static_cast<void>(FeatureGroupWeight(referenceFeature.group, objective));
            const ProbeGroup group{ metric.probeId, referenceFeature.group };
            squaredErrorSums[group] += error * error;
            ++featureCounts[group];
            ++metric.matchedFeatures;
        }
        ++metric.matchedObservations;
        ++report.matchedObservations;
    }

    std::map<std::string, double> probeWeightedMeanSquaredErrors;
    std::map<std::string, double> probeWeightSums;
    auto totalWeightedMeanSquaredError = 0.0;
    auto totalWeight = 0.0;
    for (const auto& [group, squaredErrorSum] : squaredErrorSums)
    {
        const auto weight = FeatureGroupWeight(group.second, objective);
        if (weight <= 0.0)
        {
            continue;
        }
        const auto meanSquaredError = squaredErrorSum /
            static_cast<double>(featureCounts.at(group));
        probeWeightedMeanSquaredErrors[group.first] += weight * meanSquaredError;
        probeWeightSums[group.first] += weight;
        totalWeightedMeanSquaredError += weight * meanSquaredError;
        totalWeight += weight;
    }
    if (totalWeight <= 0.0)
    {
        throw std::runtime_error(
            "Compared observations contain no feature group with a positive objective weight.");
    }
    for (auto& [probeId, metric] : metrics)
    {
        const auto weight = probeWeightSums[probeId];
        metric.normalizedRmse = weight > 0.0
            ? std::sqrt(probeWeightedMeanSquaredErrors[probeId] / weight)
            : 0.0;
        report.probes.push_back(metric);
    }
    report.weightedLoss = std::sqrt(totalWeightedMeanSquaredError / totalWeight);
    return report;
}

void SaveComparisonReport(
    const ComparisonReport& report,
    const std::filesystem::path& path)
{
    const auto parent = path.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }
    nlohmann::json document{
        { "schemaVersion", 3 }, { "experiment", report.experimentName },
        { "configurationSignature", report.configurationSignature },
        { "matchedObservations", report.matchedObservations },
        { "unmatchedReferenceObservations", report.unmatchedReferenceObservations },
        { "normalizedLoss", report.weightedLoss }, { "probes", nlohmann::json::array() }
    };
    for (const auto& metric : report.probes)
    {
        document["probes"].push_back({
            { "probe", metric.probeId }, { "matchedObservations", metric.matchedObservations },
            { "matchedFeatures", metric.matchedFeatures },
            { "normalizedRmse", metric.normalizedRmse }
        });
    }
    std::ofstream output(path);
    if (!output)
    {
        throw std::runtime_error("Could not write comparison report: " + path.string());
    }
    output << document.dump(2) << '\n';
}

bool RunComparisonSelfTest()
{
    MeasurementDataset reference{
        kDatasetSchemaVersion, "test", "configuration-001", "reference", "r1", "",
        { { "a", "default", "s", 0, "stepped-sine", "2", 48000.0, -6.0, 1000.0,
            { { "phase.degrees", "phase", "degrees", 179.0, 30.0,
                -std::numeric_limits<double>::infinity(), FeatureComparison::WrappedDegrees } } } }
    };
    auto candidate = reference;
    candidate.side = "candidate";
    candidate.observations.front().features.front().value = -179.0;
    reference.observations.front().features.push_back(
        { "harmonic.2.dbc", "harmonics", "dBc", -100.0, 6.0 });
    candidate.observations.front().features.push_back(
        { "harmonic.2.dbc", "harmonics", "dBc", -94.0, 6.0 });
    const auto report = CompareDatasets(reference, candidate, {});
    reference.observations.front().features.push_back(
        { "harmonic.3.dbc", "harmonics", "dBc", -100.0, 6.0 });
    candidate.observations.front().features.push_back(
        { "harmonic.3.dbc", "harmonics", "dBc", -94.0, 6.0 });
    const auto denseReport = CompareDatasets(reference, candidate, {});
    auto rejectedSchemaMismatch = false;
    candidate.observations.front().features.front().identifier = "different";
    try
    {
        static_cast<void>(CompareDatasets(reference, candidate, {}));
    }
    catch (const std::runtime_error&)
    {
        rejectedSchemaMismatch = true;
    }
    return report.matchedObservations == 1 && report.unmatchedReferenceObservations == 0 &&
        report.weightedLoss > 0.0 && report.weightedLoss < 1.0 &&
        std::abs(report.weightedLoss - denseReport.weightedLoss) < 1.0e-12 &&
        rejectedSchemaMismatch;
}

} // namespace emulation_lab
