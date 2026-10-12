#include "AudioFileRender.h"
#include "Capture.h"
#include "Comparison.h"
#include "Dataset.h"
#include "Diagnostics.h"
#include "ExperimentManifest.h"
#include "Host/Vst3Host.h"
#include "Optimization/EmulationOptimizer.h"
#include "Optimization/PluginParameterProblem.h"
#include "Probe.h"
#include "RunMetadata.h"
#include "TransientEnvelopeAnalysis.h"

#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>
#include <unordered_set>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

namespace
{

void RequireManifestDataset(
    const emulation_lab::ExperimentManifest& manifest,
    const emulation_lab::MeasurementDataset& dataset)
{
    if (dataset.experimentName != manifest.name ||
        dataset.configurationSignature !=
            emulation_lab::MeasurementConfigurationSignature(manifest))
    {
        throw std::runtime_error(
            "Dataset does not match the experiment measurement configuration.");
    }
}

void SaveFit(
    const emulation_lab::ExperimentManifest& manifest,
    const std::vector<double>& values,
    const std::filesystem::path& path,
    const std::string& completedStage)
{
    nlohmann::json parameters = nlohmann::json::array();
    for (std::size_t index = 0; index < values.size(); ++index)
    {
        parameters.push_back({
            { "id", manifest.candidateParameters[index].identifier },
            { "pluginParameter", manifest.candidateParameters[index].pluginParameter },
            { "value", values[index] }
        });
    }
    const nlohmann::json document{
        { "schemaVersion", 2 }, { "experiment", manifest.name },
        { "configurationSignature",
            emulation_lab::MeasurementConfigurationSignature(manifest) },
        { "completedStage", completedStage }, { "parameters", std::move(parameters) }
    };
    const auto parent = path.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }
    std::ofstream output(path);
    if (!output)
    {
        throw std::runtime_error("Could not write fit result: " + path.string());
    }
    output << document.dump(2) << '\n';
}

void LoadFit(
    const emulation_lab::ExperimentManifest& manifest,
    std::vector<double>& values,
    const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("Could not open resume result: " + path.string());
    }
    nlohmann::json document;
    input >> document;
    if (document.value("schemaVersion", 0) != 2 ||
        document.value("experiment", "") != manifest.name ||
        document.value("configurationSignature", "") !=
            emulation_lab::MeasurementConfigurationSignature(manifest))
    {
        throw std::runtime_error("Resume result does not match the experiment.");
    }
    std::unordered_set<std::string> loadedParameters;
    for (const auto& parameter : document.value("parameters", nlohmann::json::array()))
    {
        const auto identifier = parameter.value("id", "");
        const auto iterator = std::find_if(
            manifest.candidateParameters.begin(), manifest.candidateParameters.end(),
            [&identifier](const auto& descriptor) { return descriptor.identifier == identifier; });
        if (iterator == manifest.candidateParameters.end())
        {
            throw std::runtime_error("Resume result contains an unknown parameter: " + identifier);
        }
        if (!loadedParameters.insert(identifier).second)
        {
            throw std::runtime_error("Resume result repeats parameter: " + identifier);
        }
        const auto index = static_cast<std::size_t>(
            std::distance(manifest.candidateParameters.begin(), iterator));
        const auto value = parameter.value("value", values[index]);
        if (!std::isfinite(value) || value < iterator->lowerBound || value > iterator->upperBound)
        {
            throw std::runtime_error("Resume parameter is outside its bounds: " + identifier);
        }
        values[index] = value;
    }
    if (loadedParameters.size() != manifest.candidateParameters.size())
    {
        throw std::runtime_error("Resume result does not contain every candidate parameter.");
    }
}

int CaptureCommand(int argc, char** argv)
{
    if (argc != 5)
    {
        return -1;
    }
    const auto side = std::string(argv[2]);
    if (side != "reference" && side != "candidate")
    {
        return -1;
    }
    const auto manifest = emulation_lab::LoadManifest(argv[3]);
    auto capture = emulation_lab::CapturePlugin(manifest, side);
    auto outputPath = std::filesystem::path(argv[4]);
    auto environmentPath = outputPath;
    environmentPath.replace_extension(".environment.json");
    capture.dataset.environmentFile = environmentPath.filename().string();
    emulation_lab::SaveDataset(capture.dataset, outputPath);
    emulation_lab::SaveRunMetadata(capture.metadata, environmentPath);
    std::cout << "Captured observations: " << capture.dataset.observations.size() << '\n';
    return 0;
}

int FitCommand(int argc, char** argv)
{
    if (argc != 4 && argc != 5)
    {
        return -1;
    }
    const auto manifest = emulation_lab::LoadManifest(argv[2]);
    const auto reference = emulation_lab::LoadDataset(manifest.splits.training);
    RequireManifestDataset(manifest, reference);
    const auto outputPath = std::filesystem::path(argv[3]);
    if (manifest.candidateParameters.empty() || manifest.stages.empty())
    {
        throw std::runtime_error("Fit requires candidate.parameters and optimization stages.");
    }
    if (manifest.candidateModel != "vst3-parameters")
    {
        throw std::runtime_error("Unknown candidate model adapter: " + manifest.candidateModel);
    }
    std::vector<double> values;
    values.reserve(manifest.candidateParameters.size());
    for (const auto& parameter : manifest.candidateParameters)
    {
        values.push_back(parameter.initialValue);
    }
    if (argc == 5)
    {
        LoadFit(manifest, values, argv[4]);
    }

    const auto validationReference = emulation_lab::LoadDataset(manifest.splits.validation);
    RequireManifestDataset(manifest, validationReference);
    std::vector<std::string> validationObservationIds;
    validationObservationIds.reserve(validationReference.observations.size());
    for (const auto& observation : validationReference.observations)
    {
        validationObservationIds.push_back(observation.observationId);
    }
    auto guardReference = validationReference;
    std::unordered_set<std::string> includedIds(
        validationObservationIds.begin(), validationObservationIds.end());
    for (const auto& observation : reference.observations)
    {
        const auto protectedObservation = std::any_of(
            manifest.stages.begin(), manifest.stages.end(),
            [&observation](const auto& stage)
            {
                return std::any_of(stage.protectedFeatures.begin(),
                    stage.protectedFeatures.end(), [&observation](const auto& guard)
                    {
                        return observation.conditionId == guard.condition &&
                            observation.probeId == guard.probe &&
                            observation.sequenceIndex == guard.sequenceIndex;
                    });
            });
        if (protectedObservation && includedIds.insert(observation.observationId).second)
        {
            validationObservationIds.push_back(observation.observationId);
            guardReference.observations.push_back(observation);
        }
    }
    emulation_lab::PluginCaptureSession validationSession(
        manifest, "candidate", {}, std::move(validationObservationIds), true);
    const auto captureValidation = [&manifest, &validationSession](
        const std::vector<double>& candidateValues)
    {
        std::vector<emulation_lab::PluginParameterValue> assignments;
        for (std::size_t index = 0; index < candidateValues.size(); ++index)
        {
            assignments.push_back({
                manifest.candidateParameters[index].pluginParameter,
                candidateValues[index]
            });
        }
        return validationSession.Capture(assignments);
    };
    auto validationCapture = captureValidation(values);
    auto validationReport = emulation_lab::CompareDatasets(
        validationReference, validationCapture.dataset, manifest.objective);
    auto baselineReportPath = outputPath;
    baselineReportPath.replace_extension(".baseline-validation-report.json");
    emulation_lab::SaveComparisonReport(validationReport, baselineReportPath);

    for (const auto& stage : manifest.stages)
    {
        emulation_lab::PluginParameterProblem problem(
            manifest, reference, stage.parameters, stage.probes, values,
            stage.maximumObservations, stage.protectedProbes,
            stage.maximumRelativeRegression);
        std::vector<double> initial;
        for (const auto index : problem.ParameterIndices())
        {
            initial.push_back(values[index]);
        }
        emulation_lab::OptimizationOptions options;
        options.globalEvaluationLimit = manifest.optimization.globalEvaluations;
        options.localEvaluationLimit = manifest.optimization.localEvaluations;
        options.population = manifest.optimization.population;
        options.beamWidth = manifest.optimization.beamWidth;
        options.randomSeed = manifest.seed;
        const auto result = emulation_lab::Optimize(
            problem, options, { initial },
            [&manifest, &values, &problem, &outputPath, &stage](
                std::size_t evaluationCount,
                const emulation_lab::ModelCandidate& best)
            {
                if (evaluationCount % 25 != 0)
                {
                    return;
                }
                auto checkpointValues = values;
                for (std::size_t index = 0; index < best.parameterValues.size(); ++index)
                {
                    checkpointValues[problem.ParameterIndices()[index]] = best.parameterValues[index];
                }
                auto checkpointPath = outputPath;
                checkpointPath.replace_extension("." + stage.name + ".checkpoint.json");
                SaveFit(manifest, checkpointValues, checkpointPath, stage.name);
            });
        auto proposedValues = values;
        for (std::size_t index = 0; index < result.best.parameterValues.size(); ++index)
        {
            proposedValues[problem.ParameterIndices()[index]] = result.best.parameterValues[index];
        }
        auto proposedCapture = captureValidation(proposedValues);
        const auto proposedReport = emulation_lab::CompareDatasets(
            validationReference, proposedCapture.dataset, manifest.objective);
        const auto accepted = emulation_lab::AcceptStageComparison(
            validationReport, proposedReport, stage.protectedProbes,
            stage.maximumRelativeRegression) &&
            emulation_lab::AcceptFeatureGuards(
                guardReference, validationCapture.dataset, proposedCapture.dataset,
                stage.protectedFeatures);
        if (accepted)
        {
            values = std::move(proposedValues);
            validationCapture = std::move(proposedCapture);
            validationReport = proposedReport;
        }
        auto stagePath = outputPath;
        stagePath.replace_extension("." + stage.name + ".json");
        SaveFit(manifest, values, stagePath, stage.name);
        std::cout << "Completed stage " << stage.name <<
            ": objective=" << result.best.evaluation.objective <<
            ", validation=" << proposedReport.weightedLoss <<
            (accepted ? ", accepted" : ", rejected") << '\n';
    }
    SaveFit(manifest, values, outputPath, manifest.stages.back().name);
    auto candidatePath = outputPath;
    candidatePath.replace_extension(".validation-candidate.json");
    auto environmentPath = candidatePath;
    environmentPath.replace_extension(".environment.json");
    validationCapture.dataset.environmentFile = environmentPath.filename().string();
    emulation_lab::SaveDataset(validationCapture.dataset, candidatePath);
    emulation_lab::SaveRunMetadata(validationCapture.metadata, environmentPath);
    auto reportPath = outputPath;
    reportPath.replace_extension(".validation-report.json");
    emulation_lab::SaveComparisonReport(validationReport, reportPath);
    return 0;
}

} // namespace

int main(int argc, char** argv)
{
    juce::ScopedJuceInitialiser_GUI juceRuntime;
    try
    {
        if (argc == 6 && std::string(argv[1]) == "transient-envelope")
        {
            emulation_lab::AnalyzeTransientEnvelopes(argv[2], argv[3], argv[4], argv[5]);
            std::cout << "Transient envelope report: " << argv[5] << '\n';
            return 0;
        }
        if (argc == 6 && std::string(argv[1]) == "render-grid")
        {
            const auto manifest = emulation_lab::LoadManifest(argv[3]);
            emulation_lab::RenderAudioFiles(manifest, argv[2], argv[4], argv[5]);
            std::cout << "Rendered condition WAVs: " << argv[5] << '\n';
            return 0;
        }
        if (argc >= 7 && std::string(argv[1]) == "render-wav")
        {
            const auto manifest = emulation_lab::LoadManifest(argv[3]);
            std::vector<std::pair<std::string, double>> overrides;
            for (auto index = 7; index < argc; ++index)
            {
                const auto assignment = std::string(argv[index]);
                const auto separator = assignment.find('=');
                if (separator == std::string::npos)
                {
                    throw std::runtime_error("Invalid render override: " + assignment);
                }
                overrides.emplace_back(
                    assignment.substr(0, separator),
                    std::stod(assignment.substr(separator + 1)));
            }
            emulation_lab::RenderAudioFile(
                manifest, argv[2], argv[4], argv[5], argv[6], overrides);
            std::cout << "Rendered WAV: " << argv[6] << '\n';
            return 0;
        }
        if ((argc == 6 || argc == 7) && std::string(argv[1]) == "diagnose")
        {
            const auto manifest = emulation_lab::LoadManifest(argv[3]);
            emulation_lab::RunDiagnostics(
                manifest, argv[2], argv[4], argv[5],
                argc == 7 ? std::filesystem::path(argv[6]) : std::filesystem::path{});
            std::cout << "Diagnostic report: " << argv[5] << '\n';
            return 0;
        }
        if (argc == 4 && std::string(argv[1]) == "manifest" &&
            std::string(argv[2]) == "validate")
        {
            const auto manifest = emulation_lab::LoadManifest(argv[3]);
            std::cout << "Valid manifest: " << manifest.name << '\n';
            return 0;
        }
        if ((argc == 4 || argc == 5) && std::string(argv[1]) == "plugin" &&
            std::string(argv[2]) == "probe")
        {
            emulation_lab::Vst3Host host;
            std::string error;
            const auto query = argc == 5 ? std::string(argv[4]) : std::string();
            auto plugin = host.Open(argv[3], query, 44100.0, 128, error);
            if (plugin == nullptr)
            {
                throw std::runtime_error(error);
            }
            const emulation_lab::ProcessorCallbacks callbacks{
                [&plugin](std::span<const double> input) { return plugin->Process(input); },
                [&plugin] { plugin->Reset(); },
                [&plugin] { return plugin->LatencySamples(); }
            };
            emulation_lab::SteppedSineConfiguration configuration;
            configuration.minimumFrequency = 1000.0;
            configuration.maximumFrequency = 1000.0;
            configuration.minimumLevelDb = -18.0;
            configuration.maximumLevelDb = -18.0;
            const auto results = emulation_lab::SteppedSineProbe(configuration).Run(
                { 44100.0, 128, 1 }, callbacks);
            std::cout << "Processed stepped-sine observations: " << results.size() << '\n';
            return 0;
        }
        if ((argc == 4 || argc == 5) && std::string(argv[1]) == "plugin" &&
            std::string(argv[2]) == "describe")
        {
            emulation_lab::Vst3Host host;
            std::string error;
            const auto query = argc == 5 ? std::string(argv[4]) : std::string();
            auto plugin = host.Open(argv[3], query, 44100.0, 128, error);
            if (plugin == nullptr)
            {
                throw std::runtime_error(error);
            }
            std::cout << plugin->Identity() << '\n';
            for (const auto& parameter : plugin->Parameters())
            {
                std::cout << parameter.name << " = " << parameter.text <<
                    " [" << parameter.normalizedValue << "]\n";
            }
            return 0;
        }
        if ((argc == 5 || argc == 6) && std::string(argv[1]) == "plugin" &&
            std::string(argv[2]) == "parameter-manifest")
        {
            emulation_lab::Vst3Host host;
            std::string error;
            const auto query = argc == 6 ? std::string(argv[5]) : std::string();
            auto plugin = host.Open(argv[3], query, 44100.0, 128, error);
            if (plugin == nullptr)
            {
                throw std::runtime_error(error);
            }
            nlohmann::json document = nlohmann::json::array();
            for (const auto& parameter : plugin->Parameters())
            {
                document.push_back({
                    { "id", parameter.name }, { "pluginParameter", parameter.name },
                    { "lower", 0.0 }, { "upper", 1.0 },
                    { "initial", parameter.normalizedValue }
                });
            }
            std::ofstream output(argv[4]);
            if (!output)
            {
                throw std::runtime_error("Could not write parameter manifest.");
            }
            output << document.dump(2) << '\n';
            return 0;
        }
        if (argc >= 7 && std::string(argv[1]) == "plugin" &&
            std::string(argv[2]) == "parameter-scale")
        {
            emulation_lab::Vst3Host host;
            std::string error;
            const auto query = std::string(argv[5]) == "-" ? std::string() : std::string(argv[5]);
            auto plugin = host.Open(argv[3], query, 44100.0, 128, error);
            if (plugin == nullptr)
            {
                throw std::runtime_error(error);
            }
            nlohmann::json document = nlohmann::json::array();
            for (auto argument = 6; argument < argc; ++argument)
            {
                const std::string name(argv[argument]);
                nlohmann::json scale = nlohmann::json::array();
                for (auto step = 0; step <= 1000; ++step)
                {
                    const auto normalized = static_cast<double>(step) / 1000.0;
                    emulation_lab::PluginParameterDescription description;
                    if (!plugin->SetParameterNormalized(name, normalized) ||
                        !plugin->Parameter(name, description))
                    {
                        throw std::runtime_error("Unknown parameter: " + name);
                    }
                    scale.push_back({ { "normalized", description.normalizedValue },
                        { "text", description.text } });
                }
                document.push_back({ { "name", name }, { "scale", std::move(scale) } });
            }
            std::ofstream output(argv[4]);
            if (!output)
            {
                throw std::runtime_error("Could not write parameter scale.");
            }
            output << document.dump(2) << '\n';
            return 0;
        }
        if (argc >= 6 && std::string(argv[1]) == "plugin" &&
            std::string(argv[2]) == "save-state")
        {
            emulation_lab::Vst3Host host;
            std::string error;
            const auto query = std::string(argv[5]) == "-" ? std::string() : std::string(argv[5]);
            auto plugin = host.Open(argv[3], query, 44100.0, 128, error);
            if (plugin == nullptr)
            {
                throw std::runtime_error(error);
            }
            for (auto index = 6; index < argc; ++index)
            {
                const auto assignment = std::string(argv[index]);
                const auto separator = assignment.find('=');
                if (separator == std::string::npos || !plugin->SetParameterText(
                    assignment.substr(0, separator), assignment.substr(separator + 1)))
                {
                    throw std::runtime_error("Invalid plugin parameter assignment: " + assignment);
                }
            }
            if (!plugin->SaveState(argv[4]))
            {
                throw std::runtime_error("Could not save plugin state.");
            }
            return 0;
        }
        if (argc >= 2 && std::string(argv[1]) == "capture")
        {
            const auto result = CaptureCommand(argc, argv);
            if (result >= 0)
            {
                return result;
            }
        }
        if (argc == 4 && std::string(argv[1]) == "split")
        {
            const auto manifest = emulation_lab::LoadManifest(argv[2]);
            const auto source = emulation_lab::LoadDataset(argv[3]);
            RequireManifestDataset(manifest, source);
            const auto split = emulation_lab::SplitDataset(
                source, manifest.splits.validationFraction);
            emulation_lab::SaveDataset(split.first, manifest.splits.training);
            emulation_lab::SaveDataset(split.second, manifest.splits.validation);
            std::cout << "Training observations: " << split.first.observations.size() <<
                ", validation observations: " << split.second.observations.size() << '\n';
            return 0;
        }
        if (argc == 6 && std::string(argv[1]) == "compare")
        {
            const auto manifest = emulation_lab::LoadManifest(argv[2]);
            const auto reference = emulation_lab::LoadDataset(argv[3]);
            const auto candidate = emulation_lab::LoadDataset(argv[4]);
            RequireManifestDataset(manifest, reference);
            RequireManifestDataset(manifest, candidate);
            const auto report = emulation_lab::CompareDatasets(
                reference, candidate, manifest.objective);
            emulation_lab::SaveComparisonReport(report, argv[5]);
            std::cout << "Compared observations: " << report.matchedObservations << '\n';
            return report.matchedObservations > 0 ? 0 : 1;
        }
        if (argc == 6 && std::string(argv[1]) == "validate")
        {
            const auto manifest = emulation_lab::LoadManifest(argv[2]);
            const auto reference = emulation_lab::LoadDataset(argv[3]);
            const auto candidate = emulation_lab::LoadDataset(argv[4]);
            RequireManifestDataset(manifest, reference);
            RequireManifestDataset(manifest, candidate);
            const auto report = emulation_lab::CompareDatasets(
                reference, candidate, manifest.objective);
            const auto maximumLoss = std::stod(argv[5]);
            const auto passed = report.unmatchedReferenceObservations == 0 &&
                report.weightedLoss <= maximumLoss;
            std::cout << (passed ? "Validation passed" : "Validation failed") <<
                ": normalizedLoss=" << report.weightedLoss <<
                ", unmatched=" << report.unmatchedReferenceObservations << '\n';
            return passed ? 0 : 1;
        }
        if (argc >= 2 && std::string(argv[1]) == "fit")
        {
            const auto result = FitCommand(argc, argv);
            if (result >= 0)
            {
                return result;
            }
        }
    }
    catch (const std::exception& exception)
    {
        std::cerr << exception.what() << '\n';
        return 1;
    }

    std::cout << "EmulationLab commands:\n"
        "  manifest validate <path>\n"
        "  plugin probe <bundle> [query]\n"
        "  plugin describe <bundle> [query]\n"
        "  plugin parameter-manifest <bundle> <output.json> [query]\n"
        "  plugin parameter-scale <bundle> <output.json> <query|-> <name> [name ...]\n"
        "  plugin save-state <bundle> <output.state> <query|-> [Name=Text ...]\n"
        "  capture <reference|candidate> <manifest> <output.json>\n"
        "  diagnose <reference|candidate> <manifest> <condition> <output.json> [fit.json]\n"
        "  render-grid <reference|candidate> <manifest> <input.wav> <output-directory>\n"
        "  render-wav <reference|candidate> <manifest> <condition> <input.wav> <output.wav> [Name=NormalizedValue ...]\n"
        "  split <manifest> <reference-full.json>\n"
        "  compare <manifest> <reference.json> <candidate.json> <report.json>\n"
        "  validate <manifest> <reference.json> <candidate.json> <max-loss>\n"
        "  fit <manifest> <output.json> [resume.json]\n";
    return 1;
}
