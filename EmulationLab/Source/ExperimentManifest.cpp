#include "ExperimentManifest.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_set>
#include <utility>

#include <nlohmann/json.hpp>

namespace emulation_lab
{

namespace
{

using Json = nlohmann::json;

std::vector<std::string> ReadStringArray(
    const Json& object,
    const char* key)
{
    if (!object.contains(key))
    {
        return {};
    }
    if (!object.at(key).is_array())
    {
        throw std::runtime_error(std::string(key) + " must be an array of strings");
    }
    std::vector<std::string> values;
    for (const auto& value : object.at(key))
    {
        if (!value.is_string())
        {
            throw std::runtime_error(std::string(key) + " must contain only strings");
        }
        values.push_back(value.get<std::string>());
    }
    return values;
}

std::string ReadString(
    const Json& object,
    const char* key)
{
    if (!object.contains(key))
    {
        return {};
    }
    if (!object.at(key).is_string())
    {
        throw std::runtime_error(std::string(key) + " must be a string");
    }
    return object.at(key).get<std::string>();
}

std::string ResolvePath(
    const std::filesystem::path& manifestPath,
    const std::string& value)
{
    if (value.empty())
    {
        return {};
    }
    const std::filesystem::path path(value);
    return (path.is_absolute() ? path : manifestPath.parent_path() / path)
        .lexically_normal().string();
}

std::string FoldCase(std::string value)
{
    std::transform(
        value.begin(), value.end(), value.begin(),
        [](unsigned char character)
        {
            return static_cast<char>(std::tolower(character));
        });
    return value;
}

PluginControlValue ReadControlValue(
    const std::string& name,
    const Json& value)
{
    if (value.is_number())
    {
        return { name, {}, value.get<double>(), true };
    }
    if (value.is_string())
    {
        return { name, value.get<std::string>(), 0.0, false };
    }
    throw std::runtime_error(
        "plugin parameter " + name + " must be normalized number or display text");
}

void ReadSteppedSineConfiguration(
    const Json& value,
    SteppedSineConfiguration& configuration)
{
    configuration.minimumFrequency = value.value("minimumFrequency", configuration.minimumFrequency);
    configuration.maximumFrequency = value.value("maximumFrequency", configuration.maximumFrequency);
    configuration.pointsPerOctave = value.value("pointsPerOctave", configuration.pointsPerOctave);
    configuration.minimumLevelDb = value.value("minimumLevelDb", configuration.minimumLevelDb);
    configuration.maximumLevelDb = value.value("maximumLevelDb", configuration.maximumLevelDb);
    configuration.levelStepDb = value.value("levelStepDb", configuration.levelStepDb);
    configuration.minimumPreRollSeconds = value.value(
        "minimumPreRollSeconds", configuration.minimumPreRollSeconds);
    configuration.minimumMeasurementSeconds = value.value(
        "minimumMeasurementSeconds", configuration.minimumMeasurementSeconds);
    configuration.preRollCycles = value.value("preRollCycles", configuration.preRollCycles);
    configuration.measurementCycles = value.value(
        "measurementCycles", configuration.measurementCycles);
    configuration.harmonicFloorDbc = value.value(
        "harmonicFloorDbc", configuration.harmonicFloorDbc);
    configuration.maximumHarmonic = value.value(
        "maximumHarmonic", configuration.maximumHarmonic);
    configuration.resetEachPoint = value.value("resetEachPoint", configuration.resetEachPoint);
}

Json SteppedSineJson(const SteppedSineConfiguration& value)
{
    return {
        { "minimumFrequency", value.minimumFrequency },
        { "maximumFrequency", value.maximumFrequency },
        { "pointsPerOctave", value.pointsPerOctave },
        { "minimumLevelDb", value.minimumLevelDb },
        { "maximumLevelDb", value.maximumLevelDb },
        { "levelStepDb", value.levelStepDb },
        { "minimumPreRollSeconds", value.minimumPreRollSeconds },
        { "minimumMeasurementSeconds", value.minimumMeasurementSeconds },
        { "preRollCycles", value.preRollCycles },
        { "measurementCycles", value.measurementCycles },
        { "harmonicFloorDbc", value.harmonicFloorDbc },
        { "maximumHarmonic", value.maximumHarmonic },
        { "resetEachPoint", value.resetEachPoint }
    };
}

} // namespace

std::vector<std::string> ValidateManifest(const ExperimentManifest& manifest)
{
    std::vector<std::string> errors;
    if (manifest.name.empty())
    {
        errors.emplace_back("name is required");
    }
    if (manifest.referencePlugin.empty())
    {
        errors.emplace_back("reference.plugin is required");
    }
    if (manifest.candidatePlugin.empty() && manifest.candidateModel.empty())
    {
        errors.emplace_back("candidate.plugin or candidate.model is required");
    }
    if (!manifest.candidateModel.empty() && manifest.candidateModel != "vst3-parameters")
    {
        errors.emplace_back("candidate.model is not supported: " + manifest.candidateModel);
    }
    if (manifest.candidateModel == "vst3-parameters" && manifest.candidatePlugin.empty())
    {
        errors.emplace_back("vst3-parameters requires candidate.plugin");
    }
    if (!manifest.stages.empty() &&
        (manifest.candidateModel != "vst3-parameters" ||
            manifest.candidateParameters.empty()))
    {
        errors.emplace_back(
            "optimization stages require vst3-parameters and candidate parameters");
    }
    if (manifest.sampleRates.empty())
    {
        errors.emplace_back("sampleRates must contain at least one value");
    }
    if (manifest.blockSize == 0)
    {
        errors.emplace_back("blockSize must be positive");
    }
    for (const auto sampleRate : manifest.sampleRates)
    {
        if (!std::isfinite(sampleRate) || sampleRate <= 0.0)
        {
            errors.emplace_back("sampleRates must contain positive finite values");
            break;
        }
    }
    if (manifest.probes.empty())
    {
        errors.emplace_back("probes must contain at least one probe");
    }
    const std::unordered_set<std::string> knownProbes{
        "linear-sweep", "harmonic-grid", "level-sweep",
        "transient-burst", "multitone", "stereo-interaction"
    };
    std::unordered_set<std::string> probeIds;
    for (const auto& probe : manifest.probes)
    {
        if (!knownProbes.contains(probe) || !probeIds.insert(probe).second)
        {
            errors.emplace_back("probe is unknown or repeated: " + probe);
            break;
        }
    }
    if (manifest.splits.training.empty() || manifest.splits.validation.empty())
    {
        errors.emplace_back("training and validation splits are required");
    }
    std::unordered_set<std::string> stageNames;
    const auto patternMatches = [](const std::string& pattern, const std::string& value)
    {
        if (pattern == "*")
        {
            return true;
        }
        const auto wildcard = pattern.find('*');
        return wildcard == std::string::npos
            ? pattern == value
            : value.starts_with(pattern.substr(0, wildcard)) &&
                value.ends_with(pattern.substr(wildcard + 1));
    };
    for (const auto& stage : manifest.stages)
    {
        if (stage.name.empty() || stage.parameters.empty() || stage.probes.empty() ||
            stage.name.find_first_not_of(
                "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_") !=
                std::string::npos ||
            !stageNames.insert(stage.name).second)
        {
            errors.emplace_back(
                "every stage needs a unique file-safe name, parameters and probes");
            break;
        }
        for (const auto& probe : stage.probes)
        {
            if (probe != "*" &&
                std::find(manifest.probes.begin(), manifest.probes.end(), probe) ==
                    manifest.probes.end())
            {
                errors.emplace_back(
                    "optimization stage selects a probe absent from the experiment: " + probe);
                break;
            }
        }
        if (!std::isfinite(stage.maximumRelativeRegression) ||
            stage.maximumRelativeRegression < 0.0)
        {
            errors.emplace_back("stage probe regression allowance must be finite and non-negative");
        }
        for (const auto& probe : stage.protectedProbes)
        {
            if (std::find(stage.probes.begin(), stage.probes.end(), "*") == stage.probes.end() &&
                std::find(stage.probes.begin(), stage.probes.end(), probe) == stage.probes.end())
            {
                errors.emplace_back("protected probe is absent from its optimization stage: " + probe);
            }
        }
        for (const auto& guard : stage.protectedFeatures)
        {
            if (guard.condition.empty() || guard.probe.empty() || guard.feature.empty() ||
                !std::isfinite(guard.maximumErrorIncrease) || guard.maximumErrorIncrease < 0.0)
            {
                errors.emplace_back("protected feature guard is invalid");
            }
        }
        for (const auto& pattern : stage.parameters)
        {
            const auto matched = !pattern.empty() && std::any_of(
                manifest.candidateParameters.begin(), manifest.candidateParameters.end(),
                [&pattern, &patternMatches](const auto& parameter)
                {
                    return patternMatches(pattern, parameter.identifier);
                });
            if (!matched)
            {
                errors.emplace_back(
                    "optimization stage parameter pattern matches nothing: " + pattern);
                break;
            }
        }
    }
    if (!std::isfinite(manifest.splits.validationFraction) ||
        manifest.splits.validationFraction <= 0.0 ||
        manifest.splits.validationFraction >= 1.0)
    {
        errors.emplace_back("validationFraction must be between zero and one");
    }
    std::unordered_set<std::string> parameterIds;
    std::unordered_set<std::string> pluginParameters;
    for (const auto& parameter : manifest.candidateParameters)
    {
        if (parameter.identifier.empty() || parameter.pluginParameter.empty() ||
            !std::isfinite(parameter.lowerBound) || !std::isfinite(parameter.upperBound) ||
            !std::isfinite(parameter.initialValue) ||
            parameter.lowerBound < 0.0 || parameter.upperBound > 1.0 ||
            parameter.lowerBound >= parameter.upperBound ||
            parameter.initialValue < parameter.lowerBound ||
            parameter.initialValue > parameter.upperBound ||
            !parameterIds.insert(parameter.identifier).second ||
            !pluginParameters.insert(FoldCase(parameter.pluginParameter)).second)
        {
            errors.emplace_back("candidate parameters need valid normalized bounds and initial values");
            break;
        }
    }
    std::unordered_set<std::string> conditionIds;
    for (const auto& condition : manifest.conditions)
    {
        if (condition.identifier.empty() ||
            !conditionIds.insert(condition.identifier).second)
        {
            errors.emplace_back("every condition needs an identifier");
            break;
        }
        const auto validControls = [](const std::vector<PluginControlValue>& controls)
        {
            return std::all_of(
                controls.begin(), controls.end(),
                [](const PluginControlValue& control)
                {
                    return !control.name.empty() &&
                        (control.usesNormalizedValue
                            ? std::isfinite(control.normalizedValue) &&
                                control.normalizedValue >= 0.0 &&
                                control.normalizedValue <= 1.0
                            : !control.text.empty());
                });
        };
        if (!validControls(condition.referenceControls) ||
            !validControls(condition.candidateControls))
        {
            errors.emplace_back(
                "condition controls need names and either non-empty text or normalized values in the zero-to-one range");
            break;
        }
        for (const auto& control : condition.candidateControls)
        {
            if (pluginParameters.contains(FoldCase(control.name)))
            {
                errors.emplace_back(
                    "candidate condition and optimizer both write parameter: " +
                    control.name);
                break;
            }
        }
    }
    if (manifest.optimization.globalEvaluations <= 0 ||
        manifest.optimization.localEvaluations <= 0 || manifest.optimization.beamWidth == 0)
    {
        errors.emplace_back("optimization limits and beam width must be positive");
    }
    const auto validSteppedSine = [](const SteppedSineConfiguration& value)
    {
        return std::isfinite(value.minimumFrequency) && value.minimumFrequency > 0.0 &&
            std::isfinite(value.maximumFrequency) &&
            value.maximumFrequency >= value.minimumFrequency &&
            std::isfinite(value.pointsPerOctave) && value.pointsPerOctave > 0.0 &&
            std::isfinite(value.minimumLevelDb) &&
            std::isfinite(value.maximumLevelDb) &&
            value.maximumLevelDb >= value.minimumLevelDb &&
            std::isfinite(value.levelStepDb) && value.levelStepDb > 0.0 &&
            std::isfinite(value.minimumPreRollSeconds) &&
            value.minimumPreRollSeconds > 0.0 &&
            std::isfinite(value.minimumMeasurementSeconds) &&
            value.minimumMeasurementSeconds > 0.0 &&
            std::isfinite(value.preRollCycles) && value.preRollCycles > 0.0 &&
            std::isfinite(value.measurementCycles) && value.measurementCycles > 0.0 &&
            std::isfinite(value.harmonicFloorDbc) &&
            value.maximumHarmonic > 0;
    };
    const auto finiteLevels = std::all_of(
        manifest.probeConfiguration.levelSweep.levelsDb.begin(),
        manifest.probeConfiguration.levelSweep.levelsDb.end(),
        [](double value) { return std::isfinite(value); });
    const auto finiteMultitoneFrequencies = std::all_of(
        manifest.probeConfiguration.multitone.frequencies.begin(),
        manifest.probeConfiguration.multitone.frequencies.end(),
        [](double value) { return std::isfinite(value) && value > 0.0; });
    const auto minimumNyquist = manifest.sampleRates.empty()
        ? 0.0
        : *std::min_element(manifest.sampleRates.begin(), manifest.sampleRates.end()) * 0.5;
    const auto maximumMultitoneFrequency =
        manifest.probeConfiguration.multitone.frequencies.empty()
        ? 0.0
        : *std::max_element(
            manifest.probeConfiguration.multitone.frequencies.begin(),
            manifest.probeConfiguration.multitone.frequencies.end());
    const auto& levelSweep = manifest.probeConfiguration.levelSweep;
    const auto& transient = manifest.probeConfiguration.transientBurst;
    const auto finiteTransientFrequencies = std::all_of(
        transient.frequencies.begin(), transient.frequencies.end(),
        [](double value) { return std::isfinite(value) && value > 0.0; });
    const auto finiteTransientLevels = std::all_of(
        transient.levelsDb.begin(), transient.levelsDb.end(),
        [](double value) { return std::isfinite(value); });
    const auto maximumTransientFrequency = transient.frequencies.empty()
        ? 0.0
        : *std::max_element(
            transient.frequencies.begin(), transient.frequencies.end());
    const auto& multitone = manifest.probeConfiguration.multitone;
    const auto& stereo = manifest.probeConfiguration.stereoInteraction;
    if (!validSteppedSine(manifest.probeConfiguration.linearSweep) ||
        !validSteppedSine(manifest.probeConfiguration.harmonicGrid) ||
        manifest.probeConfiguration.linearSweep.maximumFrequency >= minimumNyquist ||
        manifest.probeConfiguration.harmonicGrid.maximumFrequency >= minimumNyquist ||
        levelSweep.levelsDb.empty() || !finiteLevels ||
        !std::isfinite(levelSweep.frequency) || levelSweep.frequency <= 0.0 ||
        levelSweep.frequency >= minimumNyquist ||
        !std::isfinite(levelSweep.secondsPerLevel) || levelSweep.secondsPerLevel <= 0.0 ||
        !std::isfinite(levelSweep.harmonicFloorDbc) || levelSweep.maximumHarmonic == 0 ||
        transient.frequencies.empty() || !finiteTransientFrequencies ||
        maximumTransientFrequency >= minimumNyquist ||
        transient.levelsDb.empty() || !finiteTransientLevels ||
        !std::isfinite(transient.silenceSeconds) || transient.silenceSeconds <= 0.0 ||
        !std::isfinite(transient.sustainSeconds) || transient.sustainSeconds <= 0.0 ||
        !std::isfinite(transient.tailSeconds) || transient.tailSeconds <= 0.0 ||
        !std::isfinite(transient.envelopeWindowSeconds) ||
        transient.envelopeWindowSeconds <= 0.0 ||
        multitone.frequencies.empty() || !finiteMultitoneFrequencies ||
        maximumMultitoneFrequency >= minimumNyquist || !std::isfinite(multitone.levelDb) ||
        multitone.samples < manifest.blockSize ||
        !std::isfinite(stereo.frequency) || stereo.frequency <= 0.0 ||
        stereo.frequency >= minimumNyquist || !std::isfinite(stereo.levelDb) ||
        !std::isfinite(stereo.seconds) || stereo.seconds <= 0.0)
    {
        errors.emplace_back("probe configuration is invalid");
    }
    const double weights[]{
        manifest.objective.gainDb, manifest.objective.phase,
        manifest.objective.rms, manifest.objective.harmonics,
        manifest.objective.thd, manifest.objective.peak,
        manifest.objective.envelope, manifest.objective.intermodulation,
        manifest.objective.stereo
    };
    auto totalWeight = 0.0;
    for (const auto weight : weights)
    {
        if (!std::isfinite(weight) || weight < 0.0)
        {
            errors.emplace_back("objective weights must be finite and non-negative");
            break;
        }
        totalWeight += weight;
    }
    if (totalWeight <= 0.0)
    {
        errors.emplace_back("at least one objective weight must be positive");
    }
    return errors;
}

ExperimentManifest LoadManifest(const std::filesystem::path& path)
{
    std::ifstream input(path);
    if (!input)
    {
        throw std::runtime_error("Could not open manifest: " + path.string());
    }

    Json document;
    try
    {
        input >> document;
    }
    catch (const Json::exception& exception)
    {
        throw std::runtime_error("Could not parse manifest: " +
            std::string(exception.what()));
    }

    const auto reference = document.value("reference", Json::object());
    const auto candidate = document.value("candidate", Json::object());
    const auto split = document.value("splits", Json::object());
    const auto objective = document.value("objective", Json::object());
    const auto probeSettings = document.value("probeSettings", Json::object());

    ExperimentManifest manifest;
    manifest.name = ReadString(document, "name");
    manifest.referencePlugin = ResolvePath(path, ReadString(reference, "plugin"));
    manifest.referenceState = ResolvePath(path, ReadString(reference, "state"));
    manifest.referenceQuery = ReadString(reference, "query");
    manifest.candidatePlugin = ResolvePath(path, ReadString(candidate, "plugin"));
    manifest.candidateState = ResolvePath(path, ReadString(candidate, "state"));
    manifest.candidateQuery = ReadString(candidate, "query");
    manifest.candidateModel = ReadString(candidate, "model");
    manifest.sampleRates = document.value("sampleRates", std::vector<double>{});
    manifest.blockSize = document.value("blockSize", manifest.blockSize);
    manifest.seed = document.value("seed", manifest.seed);
    manifest.probes = ReadStringArray(document, "probes");
    for (const auto& parameter : candidate.value("parameters", Json::array()))
    {
        manifest.candidateParameters.push_back({
            ReadString(parameter, "id"),
            ReadString(parameter, "pluginParameter"),
            parameter.value("lower", 0.0),
            parameter.value("upper", 1.0),
            parameter.value("initial", 0.5)
        });
    }
    for (const auto& condition : document.value("conditions", Json::array()))
    {
        ExperimentCondition parsed;
        parsed.identifier = ReadString(condition, "id");
        const auto conditionReference = condition.value("reference", Json::object());
        const auto conditionCandidate = condition.value("candidate", Json::object());
        parsed.referenceState = ResolvePath(path, ReadString(conditionReference, "state"));
        parsed.candidateState = ResolvePath(path, ReadString(conditionCandidate, "state"));
        const auto referenceControls = conditionReference.value(
            "parameters", Json::object());
        const auto candidateControls = conditionCandidate.value(
            "parameters", Json::object());
        for (const auto& control : referenceControls.items())
        {
            parsed.referenceControls.push_back(
                ReadControlValue(control.key(), control.value()));
        }
        for (const auto& control : candidateControls.items())
        {
            parsed.candidateControls.push_back(
                ReadControlValue(control.key(), control.value()));
        }
        manifest.conditions.push_back(std::move(parsed));
    }
    manifest.splits.training = ResolvePath(path, ReadString(split, "training"));
    manifest.splits.validation = ResolvePath(path, ReadString(split, "validation"));
    manifest.splits.listening = ResolvePath(path, ReadString(split, "listening"));
    manifest.splits.validationFraction = split.value(
        "validationFraction", manifest.splits.validationFraction);
    manifest.objective.gainDb = objective.value("gainDb", manifest.objective.gainDb);
    manifest.objective.phase = objective.value("phase", manifest.objective.phase);
    manifest.objective.rms = objective.value("rms", manifest.objective.rms);
    manifest.objective.harmonics = objective.value("harmonics", manifest.objective.harmonics);
    manifest.objective.thd = objective.value("thd", manifest.objective.thd);
    manifest.objective.peak = objective.value("peak", manifest.objective.peak);
    manifest.objective.envelope = objective.value("envelope", manifest.objective.envelope);
    manifest.objective.intermodulation = objective.value(
        "intermodulation",
        manifest.objective.intermodulation);
    manifest.objective.stereo = objective.value("stereo", manifest.objective.stereo);
    const auto optimization = document.value("optimization", Json::object());
    manifest.optimization.globalEvaluations = optimization.value(
        "globalEvaluations", manifest.optimization.globalEvaluations);
    manifest.optimization.localEvaluations = optimization.value(
        "localEvaluations", manifest.optimization.localEvaluations);
    manifest.optimization.population = optimization.value(
        "population", manifest.optimization.population);
    manifest.optimization.beamWidth = optimization.value(
        "beamWidth", manifest.optimization.beamWidth);
    ReadSteppedSineConfiguration(
        probeSettings.value("linear-sweep", Json::object()),
        manifest.probeConfiguration.linearSweep);
    ReadSteppedSineConfiguration(
        probeSettings.value("harmonic-grid", Json::object()),
        manifest.probeConfiguration.harmonicGrid);
    const auto levelSweep = probeSettings.value("level-sweep", Json::object());
    manifest.probeConfiguration.levelSweep.frequency = levelSweep.value(
        "frequency", manifest.probeConfiguration.levelSweep.frequency);
    manifest.probeConfiguration.levelSweep.levelsDb = levelSweep.value(
        "levelsDb", manifest.probeConfiguration.levelSweep.levelsDb);
    manifest.probeConfiguration.levelSweep.secondsPerLevel = levelSweep.value(
        "secondsPerLevel", manifest.probeConfiguration.levelSweep.secondsPerLevel);
    manifest.probeConfiguration.levelSweep.maximumHarmonic = levelSweep.value(
        "maximumHarmonic", manifest.probeConfiguration.levelSweep.maximumHarmonic);
    manifest.probeConfiguration.levelSweep.harmonicFloorDbc = levelSweep.value(
        "harmonicFloorDbc", manifest.probeConfiguration.levelSweep.harmonicFloorDbc);
    const auto transient = probeSettings.value("transient-burst", Json::object());
    manifest.probeConfiguration.transientBurst.frequencies = transient.value(
        "frequencies", manifest.probeConfiguration.transientBurst.frequencies);
    manifest.probeConfiguration.transientBurst.levelsDb = transient.value(
        "levelsDb", manifest.probeConfiguration.transientBurst.levelsDb);
    manifest.probeConfiguration.transientBurst.silenceSeconds = transient.value(
        "silenceSeconds", manifest.probeConfiguration.transientBurst.silenceSeconds);
    manifest.probeConfiguration.transientBurst.sustainSeconds = transient.value(
        "sustainSeconds", manifest.probeConfiguration.transientBurst.sustainSeconds);
    manifest.probeConfiguration.transientBurst.tailSeconds = transient.value(
        "tailSeconds", manifest.probeConfiguration.transientBurst.tailSeconds);
    manifest.probeConfiguration.transientBurst.envelopeWindowSeconds = transient.value(
        "envelopeWindowSeconds",
        manifest.probeConfiguration.transientBurst.envelopeWindowSeconds);
    const auto multitone = probeSettings.value("multitone", Json::object());
    manifest.probeConfiguration.multitone.frequencies = multitone.value(
        "frequencies", manifest.probeConfiguration.multitone.frequencies);
    manifest.probeConfiguration.multitone.levelDb = multitone.value(
        "levelDb", manifest.probeConfiguration.multitone.levelDb);
    manifest.probeConfiguration.multitone.samples = multitone.value(
        "samples", manifest.probeConfiguration.multitone.samples);
    const auto stereo = probeSettings.value("stereo-interaction", Json::object());
    manifest.probeConfiguration.stereoInteraction.frequency = stereo.value(
        "frequency", manifest.probeConfiguration.stereoInteraction.frequency);
    manifest.probeConfiguration.stereoInteraction.levelDb = stereo.value(
        "levelDb", manifest.probeConfiguration.stereoInteraction.levelDb);
    manifest.probeConfiguration.stereoInteraction.seconds = stereo.value(
        "seconds", manifest.probeConfiguration.stereoInteraction.seconds);

    for (const auto& stage : document.value("stages", Json::array()))
    {
        OptimizationStage parsedStage{
            ReadString(stage, "name"),
            ReadStringArray(stage, "parameters"),
            ReadStringArray(stage, "probes"),
            stage.value("maximumObservations", std::size_t{}),
            ReadStringArray(stage, "protectedProbes"),
            stage.value("maximumRelativeRegression", 0.02)
        };
        for (const auto& point : stage.value("protectedFeatures", Json::array()))
        {
            const auto maximumErrorIncrease = point.at("maximumErrorIncrease").get<double>();
            if (!std::isfinite(maximumErrorIncrease) || maximumErrorIncrease < 0.0)
            {
                throw std::runtime_error("Protected feature error allowance must be finite and non-negative.");
            }
            parsedStage.protectedFeatures.push_back({
                ReadString(point, "condition"), ReadString(point, "probe"),
                point.at("sequenceIndex").get<std::size_t>(),
                ReadString(point, "feature"),
                maximumErrorIncrease
            });
        }
        manifest.stages.push_back(std::move(parsedStage));
    }

    const auto errors = ValidateManifest(manifest);
    if (!errors.empty())
    {
        throw std::runtime_error("Invalid manifest: " + errors.front());
    }
    return manifest;
}

std::string MeasurementConfigurationSignature(const ExperimentManifest& manifest)
{
    Json conditions = Json::array();
    for (const auto& condition : manifest.conditions)
    {
        Json referenceControls = Json::array();
        for (const auto& control : condition.referenceControls)
        {
            referenceControls.push_back({
                { "name", control.name },
                { "mode", control.usesNormalizedValue ? "normalized" : "text" },
                { "value", control.usesNormalizedValue
                    ? Json(control.normalizedValue)
                    : Json(control.text) }
            });
        }
        Json candidateControls = Json::array();
        for (const auto& control : condition.candidateControls)
        {
            candidateControls.push_back({
                { "name", control.name },
                { "mode", control.usesNormalizedValue ? "normalized" : "text" },
                { "value", control.usesNormalizedValue
                    ? Json(control.normalizedValue)
                    : Json(control.text) }
            });
        }
        conditions.push_back({
            { "id", condition.identifier },
            { "referenceControls", std::move(referenceControls) },
            { "candidateControls", std::move(candidateControls) }
        });
    }
    if (conditions.empty())
    {
        conditions.push_back({ { "id", "default" } });
    }

    const auto& probes = manifest.probeConfiguration;
    const Json configuration{
        { "sampleRates", manifest.sampleRates },
        { "blockSize", manifest.blockSize },
        { "seed", manifest.seed },
        { "probes", manifest.probes },
        { "conditions", std::move(conditions) },
        { "linear-sweep", SteppedSineJson(probes.linearSweep) },
        { "harmonic-grid", SteppedSineJson(probes.harmonicGrid) },
        { "level-sweep", {
            { "frequency", probes.levelSweep.frequency },
            { "levelsDb", probes.levelSweep.levelsDb },
            { "secondsPerLevel", probes.levelSweep.secondsPerLevel },
            { "harmonicFloorDbc", probes.levelSweep.harmonicFloorDbc },
            { "maximumHarmonic", probes.levelSweep.maximumHarmonic }
        } },
        { "transient-burst", {
            { "frequencies", probes.transientBurst.frequencies },
            { "levelsDb", probes.transientBurst.levelsDb },
            { "silenceSeconds", probes.transientBurst.silenceSeconds },
            { "sustainSeconds", probes.transientBurst.sustainSeconds },
            { "tailSeconds", probes.transientBurst.tailSeconds },
            { "envelopeWindowSeconds", probes.transientBurst.envelopeWindowSeconds }
        } },
        { "multitone", {
            { "frequencies", probes.multitone.frequencies },
            { "levelDb", probes.multitone.levelDb },
            { "samples", probes.multitone.samples }
        } },
        { "stereo-interaction", {
            { "frequency", probes.stereoInteraction.frequency },
            { "levelDb", probes.stereoInteraction.levelDb },
            { "seconds", probes.stereoInteraction.seconds }
        } }
    };

    std::uint64_t hash = 1469598103934665603ULL;
    for (const auto value : configuration.dump())
    {
        hash ^= static_cast<unsigned char>(value);
        hash *= 1099511628211ULL;
    }
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << hash;
    return stream.str();
}

} // namespace emulation_lab
