#include "RunMetadata.h"

#include <fstream>
#include <stdexcept>

#include <nlohmann/json.hpp>

namespace emulation_lab
{

void SaveRunMetadata(
    const RunMetadata& metadata,
    const std::filesystem::path& path)
{
    const auto parent = path.parent_path();
    if (!parent.empty())
    {
        std::filesystem::create_directories(parent);
    }
    const nlohmann::json document{
        { "schemaVersion", 4 },
        { "experiment", metadata.experimentName },
        { "configurationSignature", metadata.configurationSignature },
        { "side", metadata.side },
        { "runId", metadata.runId },
        { "pluginPath", metadata.pluginPath },
        { "pluginIdentity", metadata.pluginIdentity },
        { "pluginFingerprint", metadata.pluginFingerprint },
        { "statePath", metadata.statePath },
        { "stateFingerprint", metadata.stateFingerprint },
        { "conditionIds", metadata.conditionIds },
        { "conditionStateFingerprints", metadata.conditionStateFingerprints },
        { "sampleRates", metadata.sampleRates },
        { "blockSize", metadata.blockSize },
        { "seed", metadata.seed },
        { "processingContexts", metadata.processingContexts },
        { "appliedControls", metadata.appliedControls },
        { "latencySamples", metadata.latencySamples },
        { "probes", metadata.probes }
    };
    std::ofstream output(path);
    if (!output)
    {
        throw std::runtime_error("Could not write run metadata: " + path.string());
    }
    output << document.dump(2) << '\n';
}

} // namespace emulation_lab
