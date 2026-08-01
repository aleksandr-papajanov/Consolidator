#include "c74_min.h"

#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "CommandCodec.h"
#include "DeviceHost.h"
#include "EventCodec.h"
#include "Definitions/Definitions.h"
#include "MaxDictionarySerializer.h"
#include "PersistenceCodec.h"
#include "SnapshotCodec.h"
#include "Workflows/LatestWorkflowExecutor.h"

#include <exception>
#include <utility>
#include <variant>
#include <type_traits>

using namespace c74::min;
using namespace consolidator;

class ConsolidatorDeviceHost : public object<ConsolidatorDeviceHost> {
private:
    struct RestoreTask final {
        messaging::MessageObject state;
    };

    struct RestoreResult final {
        persistence::PersistedDeviceState state;
    };

public:
    MIN_DESCRIPTION{ "Consolidator device state host." };
    MIN_TAGS{ "state, host, messaging" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> commandIn{
        this,
        "(message) commands: command 1 <source> <requestId> <name> <fields>; eq.set_parameter, eq.set_bypass, eq.set_chain_bypass <0|1>, eq.set_chain_solo <0|1>, eq.reset <bankId>, eq.reset_all, eq.join_banks <count> <bankIds...>, eq.commit_hidden <bankId>, eq.commit_all, eq.set_link <bankId> <linkId|->, eq.select_bank, gain.*, compressor.*, saturator.*, history.begin <operationId>, history.end <operationId>, history.undo, history.redo, history.restore <operationId> <undo|redo>, analyzer.clear, analyzer.set_view <0|1> <spectrum|analysis>, fit.start <pointCount> <curveDb...>, fit.complete, fit.fail; bang publishes EQ and DSP snapshots after initialization"
    };
    inlet<> persistenceIn{
        this,
        "(message) persistence: restore <dictionary> or persistence_ready"
    };

    outlet<> eventOut{
        this,
        "(anything) events/snapshots: event 1 host <eventId> <name> <fields>; snapshot 1 host eq ..."
    };
    outlet<> statusOut{
        this,
        "(anything) status: status initializing|ready|error <code>"
    };
    outlet<> debugOut{
        this,
        "(anything) diagnostics: error <code>"
    };
    outlet<> persistenceOut{
        this,
        "(dictionary) persistence state dictionary"
    };

    queue<> stateDelivery{
        this,
        MIN_FUNCTION {
            stateDeliveryScheduled = false;
            if (ready && (eqSnapshotDirty || dspSnapshotDirty ||
                processorSnapshotDirty)) {
                if (eqSnapshotDirty) PublishEqSnapshot();
                if (dspSnapshotDirty) PublishDspSnapshot();
                if (processorSnapshotDirty) PublishProcessorSnapshot();
                eqSnapshotDirty = false;
                dspSnapshotDirty = false;
                processorSnapshotDirty = false;
            }
            return {};
        }
    };

    queue<> restoreDelivery{
        this,
        MIN_FUNCTION {
            DeliverRestore();
            return {};
        }
    };

    timer<timer_options::defer_delivery> persistenceDelivery{
        this,
        MIN_FUNCTION {
            if (ready && persistenceDirty) {
                persistenceDirty = false;
                PublishPersistence();
                PublishContinuousConfirmations();
            }
            return {};
        }
    };

    message<> command{
        this,
        "command",
        "Accept a typed Host command atom list",
        MIN_FUNCTION {
            if (inlet != 0) {
                debugOut.send("error", "invalid_command_inlet");
                return {};
            }
            auto atoms = maxadapter::AtomAdapter::Read(args);
            if (atoms) atoms->insert(atoms->begin(), "command");
            ApplyCommand(atoms);
            return {};
        }
    };

    message<> list{
        this,
        "list",
        "Accept a complete Host command atom list",
        MIN_FUNCTION {
            if (inlet != 0) return {};
            const auto atoms = maxadapter::AtomAdapter::Read(args);
            if (messaging::AtomMessage::HasCategory(atoms, "command")) ApplyCommand(atoms);
            return {};
        }
    };

    message<> bang{
        this,
        "bang",
        "Publish the current Host snapshot",
        MIN_FUNCTION {
            if (!ready) {
                debugOut.send("error", "host_not_ready");
                return {};
            }
            PublishAllSnapshots();
            return {};
        }
    };

    message<> persistenceReady{
        this,
        "persistence_ready",
        "Mark persistence as restored",
        MIN_FUNCTION {
            if (inlet != 1) debugOut.send("error", "invalid_persistence_inlet");
            else if (restorePending) persistenceReadyRequested = true;
            else FinalizeInitialization();
            return {};
        }
    };

    message<> restore{
        this,
        "restore",
        "Restore a persistence dictionary",
        MIN_FUNCTION {
            if (inlet != 1 || args.size() != 1) {
                debugOut.send("error", "invalid_restore");
                return {};
            }
            const auto object = maxadapter::MaxDictionarySerializer::Deserialize<messaging::MessageObject>(args[0]);
            if (!object) {
                debugOut.send("error", "invalid_restore");
                return {};
            }
            restorePending = true;
            restoreExecutor.Submit(0, { std::move(*object) });
            return {};
        }
    };

    ConsolidatorDeviceHost()
        : host([this](const domain::Event& event) {
            if (const auto* updated = std::get_if<domain::StoreUpdatedEvent>(&event)) {
                ScheduleStatePublication(updated->storeName);
                return;
            }
            if (const auto* rejected = std::get_if<domain::CommandRejectedEvent>(&event)) {
                debugOut.send("error", rejected->code);
            }
            messaging::EventCodec::Send(event, { nextEventId++ }, [this](const messaging::AtomList& atoms) {
                eventOut.send(maxadapter::AtomAdapter::Write(atoms));
            });
        }),
          restoreExecutor(
              [](const RestoreTask& task, const workflows::WorkflowCancellation& cancellation) {
                  if (cancellation.IsRequested()) return RestoreResult{};
                  return RestoreResult{
                      persistence::PersistenceCodec::Deserialize(task.state).value_or(
                          persistence::PersistenceCodec::Defaults())
                  };
              },
              [this] { restoreDelivery.set(); }) {
        statusOut.send("status", "initializing");
    }

private:
    bool EnsureInitializedState() {
        if (!host.InstanceId().empty()) return true;
        auto defaults = persistence::PersistenceCodec::Defaults();
        if (host.Restore(
            std::move(defaults.eq),
            defaults.processor,
            0,
            defaults.instanceId
        )) {
            return true;
        }
        debugOut.send("error", "persistence_defaults_failed", host.LastRestoreError());
        return false;
    }

    void FinalizeInitialization() {
        persistenceReadyRequested = false;
        if (!EnsureInitializedState()) return;
        PublishReadyState();
        PublishAllSnapshots();
        PublishPersistence();
    }

    void DeliverRestore() {
        const auto completion = restoreExecutor.TakeCompletion();
        if (!completion || completion->error || !completion->result) {
            restorePending = false;
            debugOut.send("error", "invalid_restore");
            if (persistenceReadyRequested) FinalizeInitialization();
            return;
        }

        restorePending = false;
        auto state = std::move(completion->result->state);
        if (!host.Restore(std::move(state.eq), state.processor, 0, state.instanceId)) {
            if (!EnsureInitializedState()) {
                debugOut.send("error", "persistence_defaults_failed", host.LastRestoreError());
            }
            if (persistenceReadyRequested) FinalizeInitialization();
            return;
        }
        if (ready) PublishAllSnapshots();
        if (persistenceReadyRequested) FinalizeInitialization();
    }

    void ApplyCommand(const std::optional<messaging::AtomList>& atoms) {
        if (!atoms) {
            debugOut.send("error", "invalid_atom");
            return;
        }
        const auto decoded = messaging::CommandCodec::Decode(*atoms);
        if (!decoded.Succeeded()) {
            debugOut.send("error", decoded.error.code);
            return;
        }
        if (!ready) {
            debugOut.send("error", "host_not_ready");
            return;
        }
        try {
            const auto revisionBefore = host.Revision();
            const auto isHistoryRestore =
                std::holds_alternative<domain::UndoHistoryCommand>(decoded.command) ||
                std::holds_alternative<domain::RedoHistoryCommand>(decoded.command) ||
                std::holds_alternative<domain::RestoreHistoryOperationCommand>(decoded.command);
            suppressContinuousStatePublication = IsContinuousParameterCommand(decoded.command);
            suppressDspStatePublication = IsDspIndependentCommand(decoded.command);
            host.Handle(decoded.command);
            suppressContinuousStatePublication = false;
            suppressDspStatePublication = false;
            if (isHistoryRestore && host.Revision() != revisionBefore) {
                PublishAllSnapshots();
            }
            if (host.Revision() != revisionBefore &&
                IsContinuousParameterCommand(decoded.command)) {
                PublishParameterUpdate(decoded.command, host.Revision());
            }
        }
        catch (const std::exception&) {
            suppressContinuousStatePublication = false;
            suppressDspStatePublication = false;
            debugOut.send("error", "host_command_failed");
        }
    }

    static bool IsContinuousParameterCommand(const domain::Command& command) {
        return std::holds_alternative<domain::SetEqParameterCommand>(command) ||
            std::holds_alternative<domain::SetEqParameterIndexCommand>(command) ||
            std::holds_alternative<domain::SetGainParameterCommand>(command) ||
            std::holds_alternative<domain::SetCompressorParameterCommand>(command) ||
            std::holds_alternative<domain::SetSaturatorParameterCommand>(command);
    }

    bool IsDspIndependentCommand(const domain::Command& command) const {
        if (std::holds_alternative<domain::SetEqBankLinkCommand>(command)) return true;
        return std::holds_alternative<domain::SelectEqBankCommand>(command) &&
            !host.Eq().State().solo;
    }

    void PublishParameterUpdate(
        const domain::Command& command,
        domain::StoreRevision revision
    ) {
        const auto send = [this, revision](domain::ParameterUpdatedEvent update) {
            update.revision = revision;
            messaging::EventCodec::Send(
                domain::Event{ std::move(update) }, { nextEventId++ },
                [this](const messaging::AtomList& atoms) {
                    eventOut.send(maxadapter::AtomAdapter::Write(atoms));
                });
        };
        std::visit([&send](const auto& value) {
            using Command = std::decay_t<decltype(value)>;
            if constexpr (std::is_same_v<Command, domain::SetEqParameterCommand>) {
                send({ 0, "eq", static_cast<long>(value.bankId.value),
                    static_cast<long>(value.filterId.value), value.parameter, value.value });
            }
            else if constexpr (std::is_same_v<Command, domain::SetEqParameterIndexCommand>) {
                const auto definition = domain::FilterDefinitions().find(
                    static_cast<long>(value.filterId.value));
                if (definition == domain::FilterDefinitions().end() ||
                    value.parameterIndex >= definition->second.parameters.size()) return;
                send({ 0, "eq", static_cast<long>(value.bankId.value),
                    static_cast<long>(value.filterId.value),
                    definition->second.parameters[value.parameterIndex].name, value.value });
            }
            else if constexpr (std::is_same_v<Command, domain::SetGainParameterCommand>) {
                send({ 0, value.stage == domain::GainStage::Input ? "input_gain" : "output_gain",
                    0, 0, "gain", value.gainDb });
            }
            else if constexpr (std::is_same_v<Command, domain::SetCompressorParameterCommand>) {
                send({ 0, "compressor", 0, 0, value.parameter, value.value });
            }
            else if constexpr (std::is_same_v<Command, domain::SetSaturatorParameterCommand>) {
                send({ 0, "saturator", 0, 0, value.parameter, value.value });
            }
        }, command);
    }

    void PublishReadyState() {
        if (!ready) {
            ready = true;
            statusOut.send("status", "ready");
            messaging::EventCodec::Send(
                domain::HostInitializedEvent{ host.Revision() },
                { nextEventId++ },
                [this](const messaging::AtomList& atoms) {
                    eventOut.send(maxadapter::AtomAdapter::Write(atoms));
                });
        }
    }

    void PublishAllSnapshots() {
        eventOut.send(maxadapter::AtomAdapter::Write(
            messaging::SnapshotCodec::EncodeDevice(host.InstanceId())));
        PublishEqSnapshot();
        PublishDspSnapshot();
        PublishProcessorSnapshot();
        eqSnapshotDirty = false;
        dspSnapshotDirty = false;
        processorSnapshotDirty = false;
    }

    void PublishEqSnapshot() {
        eventOut.send(maxadapter::AtomAdapter::Write(
            messaging::SnapshotCodec::EncodeEq(host.Eq().State(), host.Revision())));
    }

    void PublishDspSnapshot() {
        eventOut.send(maxadapter::AtomAdapter::Write(
            messaging::SnapshotCodec::EncodeDsp({
                host.Revision(), host.Eq().State(), {
                    host.InputGain().State(), host.Compressor().State(),
                    host.Saturator().State(), host.OutputGain().State()
                }
            })));
    }

    void PublishProcessorSnapshot() {
        eventOut.send(maxadapter::AtomAdapter::Write(
            messaging::SnapshotCodec::EncodeProcessor({
                host.InputGain().State(), host.Compressor().State(),
                host.Saturator().State(), host.OutputGain().State()
            }, host.Revision())));
    }

    void PublishPersistence() {
        persistence::PersistedDeviceState persisted{
            persistence::PersistenceCodec::SchemaVersion,
            host.InstanceId(),
            host.Eq().State(),
            { host.InputGain().State(), host.Compressor().State(),
                host.Saturator().State(), host.OutputGain().State() }
        };
        maxadapter::MaxDictionarySerializer::Serialize(
            persistence::PersistenceCodec::Serialize(persisted),
            [this](const c74::min::atom& dictionary) {
                persistenceOut.send("dictionary", dictionary);
            });
    }

    void PublishContinuousConfirmations() {
        if (continuousEqConfirmationDirty) PublishEqSnapshot();
        if (continuousProcessorConfirmationDirty) PublishProcessorSnapshot();
        continuousEqConfirmationDirty = false;
        continuousProcessorConfirmationDirty = false;
    }

    void ScheduleStatePublication(const std::string& storeName) {
        if (!ready) return;

        if (!suppressContinuousStatePublication) {
            if (storeName == "eq") eqSnapshotDirty = true;
            else processorSnapshotDirty = true;
            if (!suppressDspStatePublication) dspSnapshotDirty = true;
            if (!stateDeliveryScheduled) {
                stateDeliveryScheduled = true;
                stateDelivery.set();
            }
        }
        else if (storeName == "eq") continuousEqConfirmationDirty = true;
        else continuousProcessorConfirmationDirty = true;

        persistenceDirty = true;
        persistenceDelivery.delay(PersistenceDebounceMilliseconds);
    }

    consolidator::host::DeviceHost host;
    workflows::LatestWorkflowExecutor<RestoreTask, RestoreResult> restoreExecutor;
    static constexpr double PersistenceDebounceMilliseconds = 100.0;
    long nextEventId = 1;
    bool ready = false;
    bool restorePending = false;
    bool persistenceReadyRequested = false;
    bool stateDeliveryScheduled = false;
    bool eqSnapshotDirty = false;
    bool dspSnapshotDirty = false;
    bool processorSnapshotDirty = false;
    bool persistenceDirty = false;
    bool suppressContinuousStatePublication = false;
    bool suppressDspStatePublication = false;
    bool continuousEqConfirmationDirty = false;
    bool continuousProcessorConfirmationDirty = false;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorDeviceHost, consolidator.devicehost);
