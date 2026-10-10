#pragma once

#include <cstddef>
#include <filesystem>
#include <string>
#include <vector>

namespace emulation_lab
{

struct RunMetadata
{
    std::string experimentName;
    std::string configurationSignature;
    std::string side;
    std::string runId;
    std::string pluginPath;
    std::string pluginIdentity;
    std::string pluginFingerprint;
    std::string statePath;
    std::string stateFingerprint;
    std::vector<std::string> conditionIds;
    std::vector<std::string> conditionStateFingerprints;
    std::vector<double> sampleRates;
    std::size_t blockSize{};
    unsigned int seed{};
    std::vector<std::string> processingContexts;
    std::vector<std::string> appliedControls;
    std::vector<std::size_t> latencySamples;
    std::vector<std::string> probes;
};

void SaveRunMetadata(
    const RunMetadata& metadata,
    const std::filesystem::path& path);

} // namespace emulation_lab
