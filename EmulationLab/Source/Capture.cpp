#include "Capture.h"

#include "Host/Vst3Host.h"
#include "Probe.h"

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>
#include <unordered_set>

namespace emulation_lab
{

namespace
{

std::string RunId()
{
    static std::atomic<std::uint64_t> sequence{};
    const auto value = std::chrono::duration_cast<std::chrono::microseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    return std::to_string(value) + "-" +
        std::to_string(sequence.fetch_add(1, std::memory_order_relaxed));
}

void HashBytes(std::uint64_t& hash, const std::filesystem::path& path)
{
    std::ifstream input(path, std::ios::binary);
    char buffer[8192];
    while (input.read(buffer, sizeof(buffer)) || input.gcount() > 0)
    {
        for (std::streamsize index = 0; index < input.gcount(); ++index)
        {
            hash ^= static_cast<unsigned char>(buffer[index]);
            hash *= 1099511628211ULL;
        }
    }
}

std::string Fingerprint(const std::filesystem::path& path)
{
    if (path.empty() || !std::filesystem::exists(path))
    {
        return {};
    }
    std::vector<std::filesystem::path> files;
    if (std::filesystem::is_regular_file(path))
    {
        files.push_back(path);
    }
    else
    {
        for (const auto& entry : std::filesystem::recursive_directory_iterator(path))
        {
            if (entry.is_regular_file())
            {
                files.push_back(entry.path());
            }
        }
        std::sort(files.begin(), files.end());
    }
    std::uint64_t hash = 1469598103934665603ULL;
    for (const auto& file : files)
    {
        const auto relative = file.lexically_relative(path).generic_string();
        for (const auto value : relative)
        {
            hash ^= static_cast<unsigned char>(value);
            hash *= 1099511628211ULL;
        }
        HashBytes(hash, file);
    }
    std::ostringstream stream;
    stream << std::hex << std::setfill('0') << std::setw(16) << hash;
    return stream.str();
}

bool Includes(const std::vector<std::string>& selected, const std::string& probe)
{
    return selected.empty() ||
        std::find(selected.begin(), selected.end(), "*") != selected.end() ||
        std::find(selected.begin(), selected.end(), probe) != selected.end();
}

std::string ObservationId(
    const std::string& condition,
    const std::string& probe,
    double sampleRate,
    std::size_t index,
    double level,
    double frequency)
{
    std::ostringstream stream;
    stream << condition << '/' << probe << '@' << std::setprecision(12) <<
        sampleRate << ':' << index << ':' << level << ':' << frequency;
    return stream.str();
}

void AppendSteppedSine(
    MeasurementDataset& dataset,
    const std::string& condition,
    const std::string& requestedId,
    const SteppedSineConfiguration& configuration,
    double sampleRate,
    const ProbeContext& context,
    const ProcessorCallbacks& processor,
    const std::unordered_set<std::string>& selectedObservationIds)
{
    SteppedSineProbe probe(configuration);
    const auto cases = probe.Cases();
    const auto sequence = condition + "-" + requestedId + "-" +
        std::to_string(static_cast<long long>(sampleRate));
    for (std::size_t index = 0; index < cases.size(); ++index)
    {
        const auto& tone = cases[index];
        const auto id = ObservationId(
            condition, requestedId, sampleRate, index, tone.levelDb, tone.frequency);
        if (!selectedObservationIds.empty() && !selectedObservationIds.contains(id))
        {
            continue;
        }
        auto result = probe.RunTone(context, processor, tone);
        dataset.observations.push_back({
            id,
            condition, sequence + "-" + std::to_string(index), 0, requestedId,
            result.probeVersion, sampleRate, tone.levelDb, tone.frequency,
            std::move(result.features)
        });
    }
}

void AppendLevelSweep(
    MeasurementDataset& dataset,
    const std::string& condition,
    const LevelSweepConfiguration& configuration,
    double sampleRate,
    const ProbeContext& context,
    const ProcessorCallbacks& processor,
    const std::unordered_set<std::string>& selectedObservationIds)
{
    auto results = LevelSweepProbe(configuration).Run(context, processor);
    const auto sequence = condition + "-level-sweep-" +
        std::to_string(static_cast<long long>(sampleRate));
    for (std::size_t index = 0; index < results.size(); ++index)
    {
        const auto level = configuration.levelsDb[index];
        const auto id = ObservationId(
            condition, "level-sweep", sampleRate, index, level, configuration.frequency);
        if (!selectedObservationIds.empty() && !selectedObservationIds.contains(id))
        {
            continue;
        }
        dataset.observations.push_back({
            id,
            condition, sequence, index, "level-sweep", results[index].probeVersion,
            sampleRate, level, configuration.frequency, std::move(results[index].features)
        });
    }
}

void AppendSingle(
    MeasurementDataset& dataset,
    const std::string& condition,
    ProbeResult result,
    double sampleRate,
    double level,
    double frequency)
{
    const auto sequence = condition + "-" + result.probeId + "-" +
        std::to_string(static_cast<long long>(sampleRate));
    dataset.observations.push_back({
        ObservationId(condition, result.probeId, sampleRate, 0, level, frequency),
        condition, sequence, 0, result.probeId, result.probeVersion,
        sampleRate, level, frequency, std::move(result.features)
    });
}

} // namespace

struct PluginCaptureSession::Impl
{
    struct Entry
    {
        std::string conditionId;
        std::string statePath;
        std::vector<PluginControlValue> controls;
        double sampleRate{};
        std::unique_ptr<Vst3Plugin> plugin;
    };

    Impl(
        ExperimentManifest sourceManifest,
        std::string sourceSide,
        std::vector<std::string> sourceProbes,
        std::vector<std::string> sourceObservationIds,
        bool collectMetadata)
        : manifest(std::move(sourceManifest)),
          side(std::move(sourceSide)),
          selectedProbes(std::move(sourceProbes)),
          selectedObservationIds(
              sourceObservationIds.begin(), sourceObservationIds.end()),
          includeMetadata(collectMetadata)
    {
        if (side != "reference" && side != "candidate")
        {
            throw std::invalid_argument("Capture side must be reference or candidate.");
        }
        pluginPath = side == "reference" ? manifest.referencePlugin : manifest.candidatePlugin;
        baseStatePath = side == "reference" ? manifest.referenceState : manifest.candidateState;
        query = side == "reference" ? manifest.referenceQuery : manifest.candidateQuery;
        if (pluginPath.empty())
        {
            throw std::runtime_error(side + ".plugin is required for plugin capture.");
        }
        auto conditions = manifest.conditions;
        if (conditions.empty())
        {
            conditions.push_back({ "default", {}, {}, {}, {} });
        }
        for (const auto& condition : conditions)
        {
            const auto conditionState = side == "reference"
                ? condition.referenceState
                : condition.candidateState;
            const auto statePath = conditionState.empty() ? baseStatePath : conditionState;
            const auto& controls = side == "reference"
                ? condition.referenceControls
                : condition.candidateControls;
            conditionIds.push_back(condition.identifier);
            conditionStateFingerprints.push_back(
                includeMetadata ? Fingerprint(statePath) : std::string{});
            for (const auto sampleRate : manifest.sampleRates)
            {
                std::string error;
                auto plugin = host.Open(
                    pluginPath, query, sampleRate,
                    static_cast<int>(manifest.blockSize), error);
                if (plugin == nullptr)
                {
                    throw std::runtime_error(error);
                }
                if (pluginIdentity.empty())
                {
                    pluginIdentity = plugin->Identity();
                }
                entries.push_back({
                    condition.identifier, statePath, controls, sampleRate, std::move(plugin)
                });
            }
        }
    }

    CaptureResult Capture(const std::vector<PluginParameterValue>& parameterValues)
    {
        CaptureResult capture;
        capture.dataset = {
            kDatasetSchemaVersion, manifest.name,
            MeasurementConfigurationSignature(manifest), side, RunId(), "", {}
        };
        capture.metadata.experimentName = manifest.name;
        capture.metadata.configurationSignature = capture.dataset.configurationSignature;
        capture.metadata.side = side;
        capture.metadata.runId = capture.dataset.runId;
        capture.metadata.pluginPath = pluginPath;
        capture.metadata.pluginIdentity = pluginIdentity;
        capture.metadata.pluginFingerprint = includeMetadata
            ? Fingerprint(pluginPath)
            : std::string{};
        capture.metadata.statePath = baseStatePath;
        capture.metadata.stateFingerprint = includeMetadata
            ? Fingerprint(baseStatePath)
            : std::string{};
        capture.metadata.conditionIds = conditionIds;
        capture.metadata.conditionStateFingerprints = conditionStateFingerprints;
        capture.metadata.sampleRates = manifest.sampleRates;
        capture.metadata.blockSize = manifest.blockSize;
        capture.metadata.seed = manifest.seed;
        capture.metadata.probes = selectedProbes.empty() ? manifest.probes : selectedProbes;

        for (auto& entry : entries)
        {
            if (!entry.statePath.empty() && !entry.plugin->LoadState(entry.statePath))
            {
                throw std::runtime_error("Could not load plugin state: " + entry.statePath);
            }
            for (const auto& control : entry.controls)
            {
                if (!entry.plugin->SetParameterText(control.name, control.text))
                {
                    throw std::runtime_error("Plugin control not found: " + control.name);
                }
            }
            for (const auto& parameter : parameterValues)
            {
                if (!entry.plugin->SetParameterNormalized(parameter.name, parameter.normalizedValue))
                {
                    throw std::runtime_error("Plugin parameter not found: " + parameter.name);
                }
            }
            entry.plugin->Reprepare();
            capture.metadata.processingContexts.push_back(
                entry.conditionId + "@" +
                std::to_string(static_cast<long long>(entry.sampleRate)));
            capture.metadata.latencySamples.push_back(entry.plugin->LatencySamples());
            const ProbeContext context{
                entry.sampleRate, manifest.blockSize, manifest.seed
            };
            const ProcessorCallbacks processor{
                [&entry](std::span<const double> input)
                {
                    return entry.plugin->Process(input);
                },
                [&entry] { entry.plugin->Reset(); },
                [&entry] { return entry.plugin->LatencySamples(); },
                [&entry](std::span<const double> left, std::span<const double> right)
                {
                    return entry.plugin->ProcessStereo(left, right);
                }
            };
            for (const auto& probeId : manifest.probes)
            {
                if (!Includes(selectedProbes, probeId))
                {
                    continue;
                }
                if (probeId == "linear-sweep" || probeId == "harmonic-grid")
                {
                    const auto& configuration = probeId == "linear-sweep"
                        ? manifest.probeConfiguration.linearSweep
                        : manifest.probeConfiguration.harmonicGrid;
                    AppendSteppedSine(
                        capture.dataset, entry.conditionId, probeId, configuration,
                        entry.sampleRate, context, processor, selectedObservationIds);
                }
                else if (probeId == "level-sweep")
                {
                    AppendLevelSweep(
                        capture.dataset, entry.conditionId,
                        manifest.probeConfiguration.levelSweep,
                        entry.sampleRate, context, processor, selectedObservationIds);
                }
                else if (probeId == "transient-burst")
                {
                    const auto& configuration = manifest.probeConfiguration.transientBurst;
                    const auto id = ObservationId(
                        entry.conditionId, probeId, entry.sampleRate, 0,
                        configuration.levelDb, configuration.frequency);
                    if (!selectedObservationIds.empty() && !selectedObservationIds.contains(id))
                    {
                        continue;
                    }
                    AppendSingle(
                        capture.dataset, entry.conditionId,
                        TransientBurstProbe(configuration).Run(context, processor),
                        entry.sampleRate, configuration.levelDb, configuration.frequency);
                }
                else if (probeId == "multitone")
                {
                    const auto& configuration = manifest.probeConfiguration.multitone;
                    const auto id = ObservationId(
                        entry.conditionId, probeId, entry.sampleRate, 0,
                        configuration.levelDb, 0.0);
                    if (!selectedObservationIds.empty() && !selectedObservationIds.contains(id))
                    {
                        continue;
                    }
                    AppendSingle(
                        capture.dataset, entry.conditionId,
                        MultitoneProbe(configuration).Run(context, processor),
                        entry.sampleRate, configuration.levelDb, 0.0);
                }
                else if (probeId == "stereo-interaction")
                {
                    const auto& configuration = manifest.probeConfiguration.stereoInteraction;
                    const auto id = ObservationId(
                        entry.conditionId, probeId, entry.sampleRate, 0,
                        configuration.levelDb, configuration.frequency);
                    if (!selectedObservationIds.empty() && !selectedObservationIds.contains(id))
                    {
                        continue;
                    }
                    AppendSingle(
                        capture.dataset, entry.conditionId,
                        StereoInteractionProbe(configuration).Run(context, processor),
                        entry.sampleRate, configuration.levelDb, configuration.frequency);
                }
            }
        }
        return capture;
    }

    ExperimentManifest manifest;
    std::string side;
    std::vector<std::string> selectedProbes;
    std::unordered_set<std::string> selectedObservationIds;
    bool includeMetadata{};
    std::string pluginPath;
    std::string baseStatePath;
    std::string query;
    std::string pluginIdentity;
    std::vector<std::string> conditionIds;
    std::vector<std::string> conditionStateFingerprints;
    Vst3Host host;
    std::vector<Entry> entries;
};

PluginCaptureSession::PluginCaptureSession(
    ExperimentManifest manifest,
    std::string side,
    std::vector<std::string> selectedProbes,
    std::vector<std::string> selectedObservationIds,
    bool includeMetadata)
    : impl_(std::make_unique<Impl>(
        std::move(manifest), std::move(side),
        std::move(selectedProbes), std::move(selectedObservationIds), includeMetadata))
{
}

PluginCaptureSession::~PluginCaptureSession() = default;

CaptureResult PluginCaptureSession::Capture(
    const std::vector<PluginParameterValue>& parameterValues)
{
    return impl_->Capture(parameterValues);
}

CaptureResult CapturePlugin(
    const ExperimentManifest& manifest,
    const std::string& side,
    const std::vector<std::string>& selectedProbes,
    const std::vector<PluginParameterValue>& parameterValues,
    bool includeMetadata)
{
    PluginCaptureSession session(manifest, side, selectedProbes, {}, includeMetadata);
    return session.Capture(parameterValues);
}

MeasurementDataset FilterDataset(
    const MeasurementDataset& source,
    const std::vector<std::string>& probes)
{
    if (probes.empty() || std::find(probes.begin(), probes.end(), "*") != probes.end())
    {
        return source;
    }
    auto filtered = source;
    filtered.observations.clear();
    for (const auto& observation : source.observations)
    {
        if (std::find(probes.begin(), probes.end(), observation.probeId) != probes.end())
        {
            filtered.observations.push_back(observation);
        }
    }
    return filtered;
}

} // namespace emulation_lab
