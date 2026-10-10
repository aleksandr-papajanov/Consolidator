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

bool AcceptStageComparison(
    const ComparisonReport& baseline,
    const ComparisonReport& candidate,
    const std::vector<std::string>& protectedProbes,
    double maximumRelativeRegression)
{
    if (candidate.matchedObservations != baseline.matchedObservations ||
        candidate.unmatchedReferenceObservations != 0 ||
        !(candidate.weightedLoss < baseline.weightedLoss))
    {
        return false;
    }
    for (const auto& probeId : protectedProbes)
    {
        const auto findProbe = [&probeId](const auto& probes)
        {
            return std::find_if(probes.begin(), probes.end(),
                [&probeId](const auto& probe) { return probe.probeId == probeId; });
        };
        const auto original = findProbe(baseline.probes);
        const auto proposed = findProbe(candidate.probes);
        if (original == baseline.probes.end() || proposed == candidate.probes.end() ||
            proposed->normalizedRmse > original->normalizedRmse *
                (1.0 + maximumRelativeRegression))
        {
            return false;
        }
    }
    return true;
}

bool AcceptFeatureGuards(
    const MeasurementDataset& reference,
    const MeasurementDataset& baseline,
    const MeasurementDataset& candidate,
    const std::vector<OptimizationStage::ProtectedFeature>& guards)
{
    for (const auto& guard : guards)
    {
        auto matched = false;
        for (const auto& observation : reference.observations)
        {
            if (observation.conditionId != guard.condition ||
                observation.probeId != guard.probe ||
                observation.sequenceIndex != guard.sequenceIndex)
            {
                continue;
            }
            matched = true;
            const auto findObservation = [&observation](const auto& dataset)
            {
                return std::find_if(dataset.observations.begin(), dataset.observations.end(),
                    [&observation](const auto& value)
                    {
                        return value.observationId == observation.observationId;
                    });
            };
            const auto previous = findObservation(baseline);
            const auto proposed = findObservation(candidate);
            const auto findFeature = [&guard](const auto& features)
            {
                return std::find_if(features.begin(), features.end(),
                    [&guard](const auto& value)
                    {
                        return value.identifier == guard.feature;
                    });
            };
            const auto targetFeature = findFeature(observation.features);
            if (previous == baseline.observations.end() ||
                proposed == candidate.observations.end() ||
                targetFeature == observation.features.end())
            {
                throw std::runtime_error("Protected feature is missing from a candidate capture.");
            }
            const auto previousFeature = findFeature(previous->features);
            const auto proposedFeature = findFeature(proposed->features);
            if (previousFeature == previous->features.end() ||
                proposedFeature == proposed->features.end())
            {
                throw std::runtime_error("Protected feature is missing from a candidate observation.");
            }
            const auto previousError = std::abs(
                FeatureError(*targetFeature, *previousFeature) * targetFeature->scale);
            const auto proposedError = std::abs(
                FeatureError(*targetFeature, *proposedFeature) * targetFeature->scale);
            if (proposedError > previousError + guard.maximumErrorIncrease)
            {
                return false;
            }
        }
        if (!matched)
        {
            throw std::runtime_error("Protected feature guard does not match reference observations.");
        }
    }
    return true;
}

} // namespace emulation_lab
