#include "Optimization/PluginParameterProblem.h"

#include "Comparison.h"

#include <algorithm>
#include <cmath>
#include <map>
#include <stdexcept>
#include <utility>

namespace emulation_lab
{

bool PatternMatches(const std::string& pattern, const std::string& value)
{
    if (pattern == "*")
    {
        return true;
    }
    const auto wildcard = pattern.find('*');
    if (wildcard == std::string::npos)
    {
        return pattern == value;
    }
    return value.starts_with(pattern.substr(0, wildcard)) &&
        value.ends_with(pattern.substr(wildcard + 1));
}

PluginParameterProblem::PluginParameterProblem(
    ExperimentManifest manifest,
    MeasurementDataset reference,
    std::vector<std::string> parameterPatterns,
    std::vector<std::string> probes,
    std::vector<double> baseValues,
    std::size_t maximumObservations,
    std::vector<std::string> protectedProbes,
    double maximumRelativeRegression)
    : manifest_(std::move(manifest)),
      reference_(FilterDataset(reference, probes)),
      probes_(std::move(probes)),
      baseValues_(std::move(baseValues)),
      protectedProbes_(std::move(protectedProbes)),
      maximumRelativeRegression_(maximumRelativeRegression)
{
    if (!std::isfinite(maximumRelativeRegression_) || maximumRelativeRegression_ < 0.0)
    {
        throw std::invalid_argument("Maximum relative probe regression must be non-negative and finite.");
    }
    if (maximumObservations > 0 && reference_.observations.size() > maximumObservations)
    {
        std::map<std::string, std::vector<DatasetObservation>> buckets;
        for (const auto& observation : reference_.observations)
        {
            buckets[observation.conditionId + "/" + observation.probeId].push_back(observation);
        }
        std::vector<DatasetObservation> selected;
        selected.reserve(maximumObservations);
        const auto perBucket = std::max<std::size_t>(
            1, (maximumObservations + buckets.size() - 1) / buckets.size());
        for (std::size_t round = 0; selected.size() < maximumObservations; ++round)
        {
            auto added = false;
            for (const auto& [key, observations] : buckets)
            {
                if (selected.size() == maximumObservations)
                {
                    break;
                }
                if (round >= observations.size())
                {
                    continue;
                }
                const auto sourceIndex = std::min(
                    observations.size() - 1,
                    (2 * round + 1) * observations.size() / (2 * perBucket));
                selected.push_back(observations[sourceIndex]);
                added = true;
            }
            if (!added)
            {
                break;
            }
        }
        reference_.observations = std::move(selected);
    }
    if (baseValues_.size() != manifest_.candidateParameters.size())
    {
        throw std::invalid_argument("Base parameter count differs from the manifest.");
    }
    for (std::size_t index = 0; index < manifest_.candidateParameters.size(); ++index)
    {
        const auto& parameter = manifest_.candidateParameters[index];
        const auto selected = std::any_of(
            parameterPatterns.begin(), parameterPatterns.end(),
            [&parameter](const auto& pattern)
            {
                return PatternMatches(pattern, parameter.identifier);
            });
        if (selected)
        {
            parameterIndices_.push_back(index);
            parameters_.push_back({
                parameter.identifier, parameter.lowerBound, parameter.upperBound
            });
        }
    }
    if (parameters_.empty())
    {
        throw std::invalid_argument("Optimization stage selects no candidate parameters.");
    }
    if (reference_.observations.empty())
    {
        throw std::invalid_argument("Optimization stage selects no reference observations.");
    }
    std::vector<std::string> observationIds;
    observationIds.reserve(reference_.observations.size());
    for (const auto& observation : reference_.observations)
    {
        observationIds.push_back(observation.observationId);
    }
    captureSession_ = std::make_unique<PluginCaptureSession>(
        manifest_, "candidate", probes_, std::move(observationIds), false);
    for (const auto& probe : protectedProbes_)
    {
        const auto present = std::any_of(reference_.observations.begin(),
            reference_.observations.end(), [&probe](const auto& observation)
            {
                return observation.probeId == probe;
            });
        if (!present)
        {
            throw std::invalid_argument("Protected probe is not in the stage: " + probe);
        }
    }
    if (!protectedProbes_.empty())
    {
        std::vector<double> initial;
        initial.reserve(parameterIndices_.size());
        for (const auto index : parameterIndices_)
        {
            initial.push_back(baseValues_[index]);
        }
        baselineProbeRmse_ = Evaluate(initial).diagnostics;
    }
}

std::span<const ModelParameter> PluginParameterProblem::Parameters() const noexcept
{
    return parameters_;
}

ModelEvaluation PluginParameterProblem::Evaluate(
    std::span<const double> parameterValues) const
{
    if (parameterValues.size() != parameterIndices_.size())
    {
        throw std::invalid_argument("Optimization candidate has the wrong parameter count.");
    }
    auto values = baseValues_;
    for (std::size_t index = 0; index < parameterValues.size(); ++index)
    {
        values[parameterIndices_[index]] = parameterValues[index];
    }
    std::vector<PluginParameterValue> assignments;
    assignments.reserve(values.size());
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        assignments.push_back({
            manifest_.candidateParameters[index].pluginParameter,
            values[index]
        });
    }
    const auto candidate = captureSession_->Capture(assignments).dataset;
    const auto report = CompareDatasets(reference_, candidate, manifest_.objective);
    if (report.unmatchedReferenceObservations != 0)
    {
        throw std::runtime_error("Candidate capture is missing reference observations.");
    }
    std::vector<double> diagnostics;
    diagnostics.reserve(report.probes.size());
    for (const auto& probe : report.probes)
    {
        diagnostics.push_back(probe.normalizedRmse);
    }
    auto objective = report.weightedLoss;
    for (std::size_t index = 0; index < report.probes.size(); ++index)
    {
        const auto& probe = report.probes[index];
        if (std::find(protectedProbes_.begin(), protectedProbes_.end(), probe.probeId) ==
            protectedProbes_.end() || baselineProbeRmse_.empty())
        {
            continue;
        }
        const auto limit = baselineProbeRmse_[index] *
            (1.0 + maximumRelativeRegression_);
        if (probe.normalizedRmse > limit)
        {
            objective += 1000.0 + 1000.0 * (probe.normalizedRmse - limit);
        }
    }
    return { objective, std::move(diagnostics) };
}

const std::vector<std::size_t>& PluginParameterProblem::ParameterIndices() const noexcept
{
    return parameterIndices_;
}

} // namespace emulation_lab
