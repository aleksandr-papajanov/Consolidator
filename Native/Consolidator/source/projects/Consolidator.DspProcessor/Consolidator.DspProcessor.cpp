#include "c74_min.h"

#include "DeviceDspRuntime.h"
#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "SnapshotCodec.h"
#include "EventCodec.h"
#include "Settings/AudioOptions.h"
#include "RealtimeSnapshotSwap.h"
#include "LatestValueTripleBuffer.h"
#include "ProcessorTelemetry.h"
#include "Workflows/LatestWorkflowExecutor.h"

#include <atomic>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

using namespace c74::min;
using namespace consolidator;

class ConsolidatorDspProcessor :
    public object<ConsolidatorDspProcessor>,
    public sample_operator<2, 4> {
private:
    static constexpr std::size_t InvalidDeviceIndex = std::numeric_limits<std::size_t>::max();

    struct RuntimeState {
        consolidator::dsp::StereoDspChain chain;
        std::size_t compressorIndex = InvalidDeviceIndex;
        std::size_t saturatorIndex = InvalidDeviceIndex;
        std::size_t inputGainIndex = InvalidDeviceIndex;
        std::size_t outputGainIndex = InvalidDeviceIndex;
    };

    struct TopologyTask final {
        domain::StoreRevision revision = 0;
        std::vector<dsp::DspDeviceRegistration> registrations;
    };

    struct TopologyResult final {
        domain::StoreRevision revision = 0;
        std::unique_ptr<RuntimeState> runtime;
    };

public:
    MIN_DESCRIPTION{ "Consolidator complete audio processing chain." };
    MIN_TAGS{ "audio, dsp, processor" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> inputLeft{ this, "(signal) left input", "signal" };
    inlet<> inputRight{ this, "(signal) right input", "signal" };
    inlet<> commandsIn{
        this,
        "(message) Host state: snapshot 1 host dsp <revision> <EQ fields...> <input gain> <compressor fields> <saturator fields> <output gain>; event 1 host ... parameter.updated is applied directly"
    };

    outlet<> outputLeft{ this, "(signal) left output", "signal" };
    outlet<> outputRight{ this, "(signal) right output", "signal" };
    outlet<> eqInputLeft{ this, "(signal) left signal at the EQ input", "signal" };
    outlet<> eqInputRight{ this, "(signal) right signal at the EQ input", "signal" };
    outlet<> statusOut{ this, "(anything) status: status ready|error <code>" };
    outlet<> debugOut{ this, "(anything) diagnostics: error <code>" };
    outlet<> telemetryOut{
        this,
        "(anything) processor_telemetry <compressorReductionDb> <saturationNonlinearRatio> <saturationLevelDeltaDb> <inputPreDb> <inputPostDb> <outputPreDb> <outputPostDb> <compressorOutputDb> <saturatorOutputDb>"
    };

    queue<> telemetryDelivery{
        this,
        MIN_FUNCTION {
            PublishTelemetry();
            return {};
        }
    };

    queue<> topologyDelivery{
        this,
        MIN_FUNCTION {
            DeliverTopology();
            return {};
        }
    };

    ConsolidatorDspProcessor()
        : topologyExecutor(
            [this](const TopologyTask& task, const workflows::WorkflowCancellation& cancellation) {
                return BuildTopology(task, cancellation);
            },
            [this] { topologyDelivery.set(); }) {}

    message<> dspSetup{
        this,
        "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                sampleRate = static_cast<double>(args[0]);
                RebuildTopology();
                statusOut.send("status", "ready");
            }
            return {};
        }
    };

    message<> snapshotMessage{
        this,
        "snapshot",
        "Apply a complete DSP snapshot",
        MIN_FUNCTION {
            if (inlet != 2) {
                debugOut.send("error", "invalid_snapshot_inlet");
                return {};
            }
            auto atoms = maxadapter::AtomAdapter::Read(args);
            if (atoms) atoms->insert(atoms->begin(), "snapshot");
            if (messaging::AtomMessage::HasSnapshotStore(atoms, "dsp")) ApplySnapshot(atoms);
            return {};
        }
    };

    message<> list{
        this,
        "list",
        "Receive a complete DSP snapshot atom list",
        MIN_FUNCTION {
            if (inlet != 2) return {};
            const auto atoms = maxadapter::AtomAdapter::Read(args);
            if (messaging::AtomMessage::HasSnapshotStore(atoms, "dsp")) ApplySnapshot(atoms);
            return {};
        }
    };

    message<> eventMessage{
        this,
        "event",
        "Apply a short Host parameter.updated event",
        MIN_FUNCTION {
            if (inlet != 2) return {};
            auto atoms = maxadapter::AtomAdapter::Read(args);
            if (atoms) atoms->insert(atoms->begin(), "event");
            ApplyParameterEvent(atoms);
            return {};
        }
    };

    samples<4> operator()(sample left, sample right) {
        audio::StereoSample eqInput{ left, right };
        const auto output = runtimeState.Read([this, left, right, &eqInput](RuntimeState& runtime) {
            const auto observer = [this, &runtime, &eqInput](
                int channel,
                std::size_t index,
                double input,
                double output,
                const dsp::DspDeviceTelemetry& deviceTelemetry,
                bool bypassed
            ) {
                if (index == runtime.inputGainIndex) {
                    telemetry.ObserveInputGain(input, output);
                }
                else if (index == runtime.saturatorIndex) {
                    if (bypassed) telemetry.ResetSaturator();
                    else telemetry.ObserveSaturator(input, output);
                }
                else if (index == runtime.compressorIndex) {
                    if (bypassed) telemetry.ResetCompressor();
                    else telemetry.ObserveCompressor(output, deviceTelemetry.gainReductionDb);
                    if (channel == 0) eqInput.left = output;
                    else eqInput.right = output;
                }
                else if (index == runtime.outputGainIndex) {
                    telemetry.ObserveOutputGain(input, output);
                }
            };
            return runtime.chain.ProcessSampleObservedStereo({ left, right }, observer);
        });
        if (telemetry.Advance()) {
            if (!telemetry.IsSilent()) {
                telemetryFrames.ProducerValue() = telemetry.Finish();
                telemetryFrames.Publish();
                ScheduleTelemetryDelivery();
            }
            else {
                telemetry.Reset();
            }
        }
        return { output.left, output.right, eqInput.left, eqInput.right };
    }

private:
    void ApplySnapshot(const std::optional<messaging::AtomList>& atoms) {
        const auto snapshot = atoms ? messaging::SnapshotCodec::DecodeDsp(*atoms) : std::nullopt;
        if (!snapshot) {
            debugOut.send("error", "invalid_dsp_snapshot");
            return;
        }
        if (snapshot->revision < latestRevision) return;
        dspRuntime.SetSnapshot(*snapshot);
        latestRevision = snapshot->revision;
        hasSnapshot = true;
        ApplyRuntimeUpdate();
        statusOut.send("status", "ready");
    }

    void ApplyParameterEvent(const std::optional<messaging::AtomList>& atoms) {
        const auto decoded = atoms ? messaging::EventCodec::Decode(*atoms)
            : messaging::DecodedEvent{};
        const auto* update = decoded.Succeeded()
            ? std::get_if<domain::ParameterUpdatedEvent>(&decoded.event)
            : nullptr;
        if (!update) return;
        if (update->revision < latestRevision) return;

        const auto updated = hasRuntime && runtimeState.UpdateCurrent(
            [this, update](RuntimeState& runtime) {
                if (update->device == "eq") {
                    return dspRuntime.UpdateEqParameter(
                        runtime.chain, update->bankId, update->filterId,
                        update->parameter, update->value, sampleRate);
                }
                if (update->device == "input_gain") {
                    return dspRuntime.UpdateGain(runtime.chain, true, update->value, sampleRate);
                }
                if (update->device == "output_gain") {
                    return dspRuntime.UpdateGain(runtime.chain, false, update->value, sampleRate);
                }
                if (update->device == "compressor") {
                    return dspRuntime.UpdateCompressorParameter(
                        runtime.chain, update->parameter, update->value, sampleRate);
                }
                if (update->device == "saturator") {
                    return dspRuntime.UpdateSaturatorParameter(
                        runtime.chain, update->parameter, update->value, sampleRate);
                }
                return false;
            });
        if (!updated) {
            debugOut.send("error", "invalid_parameter_update");
            return;
        }
        latestRevision = update->revision;
        if (topologyPending) RebuildTopology();
    }

    static std::size_t FindDeviceIndex(
        const std::vector<dsp::DspDeviceRegistration>& registrations,
        std::string_view deviceId
    ) {
        for (std::size_t index = 0; index < registrations.size(); ++index) {
            if (registrations[index].deviceId == deviceId) return index;
        }
        return InvalidDeviceIndex;
    }

    void ApplyRuntimeUpdate() {
        const auto registrations = dspRuntime.BuildRegistrations(sampleRate);
        const auto updated = hasRuntime && runtimeState.UpdateCurrent(
            [&registrations](RuntimeState& runtime) {
                return runtime.chain.Update(registrations);
            });
        if (updated) return;
        RebuildTopology(registrations);
    }

    void RebuildTopology() {
        if (!hasSnapshot) return;
        RebuildTopology(dspRuntime.BuildRegistrations(sampleRate));
    }

    void RebuildTopology(const std::vector<dsp::DspDeviceRegistration>& registrations) {
        topologyPending = true;
        requestedTopologyRevision = latestRevision;
        topologyExecutor.Submit(requestedTopologyRevision, {
            requestedTopologyRevision, registrations
        });
    }

    TopologyResult BuildTopology(
        const TopologyTask& task,
        const workflows::WorkflowCancellation& cancellation
    ) const {
        TopologyResult result;
        result.revision = task.revision;
        if (cancellation.IsRequested()) return result;
        dsp::DspChainBuilder builder;
        builder.SetDevices(task.registrations);
        auto runtime = std::make_unique<RuntimeState>();
        runtime->chain = builder.BuildStereo();
        runtime->compressorIndex = FindDeviceIndex(task.registrations, "compressor");
        runtime->saturatorIndex = FindDeviceIndex(task.registrations, "saturator");
        runtime->inputGainIndex = FindDeviceIndex(task.registrations, "input_gain");
        runtime->outputGainIndex = FindDeviceIndex(task.registrations, "output_gain");
        if (!cancellation.IsRequested()) result.runtime = std::move(runtime);
        return result;
    }

    void DeliverTopology() {
        auto completion = topologyExecutor.TakeCompletion();
        if (!completion || completion->error || !completion->result ||
            !completion->result->runtime) {
            if (completion && completion->error) debugOut.send("error", "dsp_topology_failed");
            return;
        }
        auto result = std::move(*completion->result);
        if (result.revision < requestedTopologyRevision) return;
        runtimeState.Replace(std::move(result.runtime));
        hasRuntime = true;
        topologyPending = false;
    }

    void PublishTelemetry() {
        telemetryFrames.ConsumeLatest([this](const ProcessorTelemetryFrame& frame) {
            telemetryOut.send(
                "processor_telemetry",
                frame.compressorReductionDb,
                frame.saturationNonlinearRatio,
                frame.saturationLevelDeltaDb,
                frame.inputPreDb,
                frame.inputPostDb,
                frame.outputPreDb,
                frame.outputPostDb,
                frame.compressorOutputDb,
                frame.saturatorOutputDb);
        });
        telemetryDeliveryScheduled.store(false, std::memory_order_release);
        if (telemetryFrames.HasPending()) ScheduleTelemetryDelivery();
    }

    void ScheduleTelemetryDelivery() {
        if (!telemetryDeliveryScheduled.exchange(true, std::memory_order_acq_rel)) {
            telemetryDelivery.set();
        }
    }

    double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz;
    consolidator::dspcore::DeviceDspRuntime dspRuntime;
    consolidator::dspcore::RealtimeSnapshotSwap<RuntimeState> runtimeState;
    consolidator::dspcore::LatestValueTripleBuffer<ProcessorTelemetryFrame> telemetryFrames;
    workflows::LatestWorkflowExecutor<TopologyTask, TopologyResult> topologyExecutor;
    ProcessorTelemetryAccumulator telemetry;
    std::atomic<bool> telemetryDeliveryScheduled{ false };
    bool hasSnapshot = false;
    bool hasRuntime = false;
    bool topologyPending = false;
    domain::StoreRevision latestRevision = 0;
    domain::StoreRevision requestedTopologyRevision = 0;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorDspProcessor, consolidator.dspprocessor);
