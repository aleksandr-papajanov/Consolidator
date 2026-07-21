#include "c74_min.h"

#include "DeviceDspRuntime.h"
#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "SnapshotCodec.h"
#include "Settings/AudioOptions.h"
#include "RealtimeSnapshotSwap.h"

#include <memory>
#include <optional>

using namespace c74::min;
using namespace consolidator;

class ConsolidatorDspProcessor :
    public object<ConsolidatorDspProcessor>,
    public sample_operator<2, 2> {
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
    outlet<> statusOut{ this, "(anything) status: status ready|error <code>" };
    outlet<> debugOut{ this, "(anything) diagnostics: error <code>" };

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

    samples<2> operator()(sample left, sample right) {
        const auto output = runtimeState.Read([left, right](RuntimeState& runtime) {
            return runtime.chain.ProcessSample({ left, right });
        });
        return { output.left, output.right };
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
    };

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
        runtimeState.Replace(std::move(runtime));
        hasRuntime = true;
    }

    double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz;
    consolidator::dspcore::DeviceDspRuntime dspRuntime;
    consolidator::dspcore::RealtimeSnapshotSwap<RuntimeState> runtimeState;
    bool hasSnapshot = false;
    bool hasRuntime = false;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorDspProcessor, consolidator.dspprocessor);
