#include "Optimization/PluginParameterProblem.h"

#include "Comparison.h"

#include <algorithm>
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
    std::size_t maximumObservations)
    : manifest_(std::move(manifest)),
      reference_(FilterDataset(reference, probes)),
      probes_(std::move(probes)),
      baseValues_(std::move(baseValues))
{
    if (maximumObservations > 0 && reference_.observations.size() > maximumObservations)
    {
        std::vector<DatasetObservation> selected;
        selected.reserve(maximumObservations);
        for (std::size_t index = 0; index < maximumObservations; ++index)
        {
            const auto sourceIndex = index * (reference_.observations.size() - 1) /
                std::max<std::size_t>(maximumObservations - 1, 1);
            selected.push_back(reference_.observations[sourceIndex]);
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
    return { report.weightedLoss, std::move(diagnostics) };
}

const std::vector<std::size_t>& PluginParameterProblem::ParameterIndices() const noexcept
{
    return parameterIndices_;
}

} // namespace emulation_lab
