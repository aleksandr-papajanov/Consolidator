#pragma once

#include "Probe.h"

#include <string>
#include <vector>

#include <cstddef>
#include <filesystem>

namespace emulation_lab
{

struct ObjectiveWeights
{
    double gainDb{ 1.0 };
    double phase{ 0.05 };
    double rms{ 0.35 };
    double harmonics{ 1.0 };
    double thd{ 0.35 };
    double peak{ 0.5 };
    double envelope{ 1.0 };
    double intermodulation{ 0.4 };
    double stereo{ 0.25 };
};

struct DatasetSplit
{
    std::string training;
    std::string validation;
    std::string listening;
    double validationFraction{ 0.2 };
};

struct OptimizationStage
{
    std::string name;
    std::vector<std::string> parameters;
    std::vector<std::string> probes;
    std::size_t maximumObservations{};
};

struct CandidateParameter
{
    std::string identifier;
    std::string pluginParameter;
    double lowerBound{};
    double upperBound{ 1.0 };
    double initialValue{ 0.5 };
};

struct PluginControlValue
{
    std::string name;
    std::string text;
};

struct ExperimentCondition
{
    std::string identifier;
    std::string referenceState;
    std::string candidateState;
    std::vector<PluginControlValue> referenceControls;
    std::vector<PluginControlValue> candidateControls;
};

struct OptimizationConfiguration
{
    int globalEvaluations{ 1000 };
    int localEvaluations{ 250 };
    unsigned int population{};
    std::size_t beamWidth{ 8 };
};

struct ProbeConfiguration
{
    SteppedSineConfiguration linearSweep{
        20.0, 20000.0, 12.0, -30.0, -30.0, 1.0,
        0.5, 0.5, 12.0, 20.0, -180.0, 1, true
    };
    SteppedSineConfiguration harmonicGrid;
    LevelSweepConfiguration levelSweep;
    TransientBurstConfiguration transientBurst;
    MultitoneConfiguration multitone;
    StereoInteractionConfiguration stereoInteraction;
};

struct ExperimentManifest
{
    std::string name;
    std::string referencePlugin;
    std::string referenceState;
    std::string referenceQuery;
    std::string candidatePlugin;
    std::string candidateState;
    std::string candidateQuery;
    std::string candidateModel;
    std::vector<double> sampleRates;
    std::size_t blockSize{ 128 };
    unsigned int seed{ 1 };
    std::vector<std::string> probes;
    std::vector<CandidateParameter> candidateParameters;
    std::vector<ExperimentCondition> conditions;
    DatasetSplit splits;
    ObjectiveWeights objective;
    OptimizationConfiguration optimization;
    ProbeConfiguration probeConfiguration;
    std::vector<OptimizationStage> stages;
};

[[nodiscard]] std::vector<std::string> ValidateManifest(
    const ExperimentManifest& manifest);

[[nodiscard]] ExperimentManifest LoadManifest(
    const std::filesystem::path& path);

[[nodiscard]] std::string MeasurementConfigurationSignature(
    const ExperimentManifest& manifest);

[[nodiscard]] bool RunManifestSelfTest();

} // namespace emulation_lab
