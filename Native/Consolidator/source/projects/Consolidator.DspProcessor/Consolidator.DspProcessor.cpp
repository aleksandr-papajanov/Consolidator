#include "c74_min.h"

#include "DeviceDspRuntime.h"
#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "SnapshotCodec.h"
#include "Settings/AudioOptions.h"
#include "RealtimeSnapshotSwap.h"
#include "LatestValueTripleBuffer.h"
#include "ProcessorTelemetry.h"

#include <atomic>
#include <limits>
#include <memory>
#include <optional>
#include <string_view>

using namespace c74::min;
using namespace consolidator;

class ConsolidatorDspProcessor :
    public object<ConsolidatorDspProcessor>,
    public sample_operator<2, 4> {
public:
    MIN_DESCRIPTION{ "Consolidator complete audio processing chain." };
    MIN_TAGS{ "audio, dsp, processor" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> inputLeft{ this, "(signal) left input", "signal" };
    inlet<> inputRight{ this, "(signal) right input", "signal" };
    inlet<> commandsIn{
        this,
        "(message) Host bus: snapshot 1 host dsp <revision> <EQ fields...> <input gain> <compressor fields> <saturator fields> <output gain>; event 1 host ... is ignored"
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
        "Ignore Host events not owned by the audio processor",
        MIN_FUNCTION {
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
                const dsp::DspDeviceTelemetry& deviceTelemetry
            ) {
                if (index == runtime.inputGainIndex) telemetry.ObserveInputGain(input, output);
                else if (index == runtime.saturatorIndex) telemetry.ObserveSaturator(input, output);
                else if (index == runtime.compressorIndex) {
                    telemetry.ObserveCompressor(output, deviceTelemetry.gainReductionDb);
                    if (channel == 0) eqInput.left = output;
                    else eqInput.right = output;
                }
                else if (index == runtime.outputGainIndex) telemetry.ObserveOutputGain(input, output);
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
        dspRuntime.SetSnapshot(*snapshot);
        hasSnapshot = true;
        ApplyRuntimeUpdate();
        statusOut.send("status", "ready");
    }

    struct RuntimeState {
        consolidator::dsp::StereoDspChain chain;
        std::size_t compressorIndex = InvalidDeviceIndex;
        std::size_t saturatorIndex = InvalidDeviceIndex;
        std::size_t inputGainIndex = InvalidDeviceIndex;
        std::size_t outputGainIndex = InvalidDeviceIndex;
    };

    static constexpr std::size_t InvalidDeviceIndex = std::numeric_limits<std::size_t>::max();

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
        dsp::DspChainBuilder builder;
        builder.SetDevices(registrations);
        auto runtime = std::make_unique<RuntimeState>();
        runtime->chain = builder.BuildStereo();
        runtime->compressorIndex = FindDeviceIndex(registrations, "compressor");
        runtime->saturatorIndex = FindDeviceIndex(registrations, "saturator");
        runtime->inputGainIndex = FindDeviceIndex(registrations, "input_gain");
        runtime->outputGainIndex = FindDeviceIndex(registrations, "output_gain");
        runtimeState.Replace(std::move(runtime));
        hasRuntime = true;
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
    ProcessorTelemetryAccumulator telemetry;
    std::atomic<bool> telemetryDeliveryScheduled{ false };
    bool hasSnapshot = false;
    bool hasRuntime = false;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorDspProcessor, consolidator.dspprocessor);
