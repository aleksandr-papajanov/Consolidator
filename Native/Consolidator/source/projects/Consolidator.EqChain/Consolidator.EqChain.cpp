#include "c74_min.h"

#include "DSP/Eq/EqRuntime.h"
#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "SnapshotCodec.h"
#include "Settings/AudioOptions.h"
#include "RealtimeSnapshotSwap.h"

#include <memory>
#include <optional>

using namespace c74::min;
using namespace consolidator;

class ConsolidatorEqChain :
    public object<ConsolidatorEqChain>,
    public sample_operator<2, 2> {
public:
    MIN_DESCRIPTION{ "Consolidator EQ chain audio processor." };
    MIN_TAGS{ "audio, eq, chain" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> inputLeft{ this, "(signal) left input", "signal" };
    inlet<> inputRight{ this, "(signal) right input", "signal" };
    inlet<> commandsIn{
        this,
        "(message) Host bus: snapshot 1 host eq <revision> <selectedBank> <bankCount> <banks...>; event 1 host ... is ignored"
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
                RebuildRuntime();
                statusOut.send("status", "ready");
            }
            return {};
        }
    };

    message<> snapshotMessage{
        this,
        "snapshot",
        "Apply a complete EQ snapshot",
        MIN_FUNCTION {
            if (inlet != 2) {
                debugOut.send("error", "invalid_snapshot_inlet");
                return {};
            }
            auto atoms = maxadapter::AtomAdapter::Read(args);
            if (atoms) atoms->insert(atoms->begin(), "snapshot");
            if (messaging::AtomMessage::HasSnapshotStore(atoms, "eq")) ApplySnapshot(atoms);
            return {};
        }
    };

    message<> list{
        this,
        "list",
        "Receive a complete EQ snapshot atom list",
        MIN_FUNCTION {
            if (inlet != 2) return {};
            const auto atoms = maxadapter::AtomAdapter::Read(args);
            if (messaging::AtomMessage::HasSnapshotStore(atoms, "eq")) ApplySnapshot(atoms);
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
        const auto snapshot = atoms ? messaging::SnapshotCodec::DecodeEq(*atoms) : std::nullopt;
        if (!snapshot) {
            debugOut.send("error", "invalid_eq_snapshot");
            return;
        }
        eqRuntime.SetSnapshot(*snapshot);
        RebuildRuntime();
        statusOut.send("status", "ready");
    }

    struct RuntimeState {
        consolidator::dsp::StereoDspChain chain;
    };

    void RebuildRuntime() {
        auto runtime = std::make_unique<RuntimeState>();
        runtime->chain = eqRuntime.BuildAllBanks(sampleRate).BuildStereo();
        runtimeState.Replace(std::move(runtime));
    }

    double sampleRate = consolidator::settings::AudioOptions::DefaultSampleRateHz;
    consolidator::dsp::EqRuntime eqRuntime;
    consolidator::dspcore::RealtimeSnapshotSwap<RuntimeState> runtimeState;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorEqChain, consolidator.eqchain);
