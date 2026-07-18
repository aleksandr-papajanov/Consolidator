#include "c74_min.h"

#include "DSP/Eq/EqState.h"
#include "MaxEqSnapshotAdapter.h"
#include "MaxFilterDefinitionAdapter.h"
#include "MaxMessageAdapter.h"
#include "Messaging/MessageRegistry.h"
#include "Messaging/Messages/EqSnapshotMessage.h"
#include "Messaging/Messages/FilterDefinitionMessage.h"
#include "Settings/GlobalSettings.h"

#include <atomic>
#include <memory>

using namespace c74::min;

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
        "(message) commands: message <dictionary type=filter.define|eq.storage.snapshot>"
    };

    outlet<> outputLeft{ this, "(signal) left output", "signal" };
    outlet<> outputRight{ this, "(signal) right output", "signal" };
    outlet<> debugOut{ this, "(anything) diagnostics: error <code>" };

    message<> dspSetup{
        this,
        "dspsetup",
        MIN_FUNCTION {
            if (!args.empty()) {
                sampleRate = static_cast<double>(args[0]);
                RebuildRuntime();
            }
            return {};
        }
    };

    message<> envelopeMessage{
        this,
        "message",
        "Apply a structured control envelope",
        MIN_FUNCTION {
            if (inlet != 2 || args.size() != 1) {
                debugOut.send("error", "invalid_message_envelope");
                return {};
            }
            const auto envelope = consolidator::maxadapter::MaxMessageAdapter::Deserialize(args[0]);
            if (!envelope || !consolidator::maxadapter::MaxMessageAdapter::IsAddressedTo(*envelope, "eq.chain")) {
                if (!envelope) debugOut.send("error", "invalid_message_envelope");
                return {};
            }
            const auto command = messageFactory.Deserialize(*envelope);
            if (const auto* definition = dynamic_cast<consolidator::messaging::FilterDefinitionMessage*>(command.get())) {
                Handle(*definition);
            }
            else if (const auto* snapshot = dynamic_cast<consolidator::messaging::EqSnapshotMessage*>(command.get())) {
                Handle(*snapshot);
            }
            else {
                debugOut.send("error", "invalid_message_envelope");
            }
            return {};
        }
    };

    samples<2> operator()(sample left, sample right) {
        const auto runtime = runtimeState.load(std::memory_order_acquire);
        const auto output = runtime->chain.ProcessSample({ left, right });
        return { output.left, output.right };
    }

private:
    struct RuntimeState {
        consolidator::dsp::StereoDspChain chain;
    };

    void Handle(const consolidator::messaging::FilterDefinitionMessage& command) {
        const auto definition = consolidator::maxadapter::MaxFilterDefinitionAdapter::Read(
            command.contractName, command.filterId, command.defaultBypass);
        if (!definition) {
            debugOut.send("error", "invalid_filter_definition");
            return;
        }
        eqState.Define(*definition);
        RebuildRuntime();
    }

    void Handle(const consolidator::messaging::EqSnapshotMessage& command) {
        const auto snapshot = consolidator::maxadapter::MaxEqSnapshotAdapter::Read(
            command.snapshotName, command.selectedBankId);
        if (!snapshot) {
            debugOut.send("error", "invalid_eq_storage_snapshot");
            return;
        }
        eqState.SetSnapshot(*snapshot);
        RebuildRuntime();
    }

    void RebuildRuntime() {
        auto runtime = std::make_shared<RuntimeState>();
        runtime->chain = eqState.BuildAllBanks(sampleRate).BuildStereo();
        runtimeState.store(std::move(runtime), std::memory_order_release);
    }

    double sampleRate = consolidator::settings::GlobalSettings::DefaultSampleRateHz;
    consolidator::dsp::EqState eqState;
    consolidator::messaging::MessageFactory messageFactory =
        consolidator::messaging::MessageRegistry::CreateFactory();
    std::atomic<std::shared_ptr<RuntimeState>> runtimeState{ std::make_shared<RuntimeState>() };
};

MIN_EXTERNAL_CUSTOM(ConsolidatorEqChain, consolidator.eqchain);
