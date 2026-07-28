#include "c74_min.h"

#include "AtomAdapter.h"
#include "AtomMessage.h"
#include "CommandCodec.h"
#include "DeviceHost.h"
#include "EventCodec.h"
#include "MaxDictionarySerializer.h"
#include "PersistenceCodec.h"
#include "SnapshotCodec.h"

#include <exception>
#include <utility>
#include <variant>

using namespace c74::min;
using namespace consolidator;

class ConsolidatorDeviceHost : public object<ConsolidatorDeviceHost> {
public:
    MIN_DESCRIPTION{ "Consolidator device state host." };
    MIN_TAGS{ "state, host, messaging" };
    MIN_AUTHOR{ "Oleksandr Papaianov" };

    inlet<> commandIn{
        this,
        "(message) commands: command 1 <source> <requestId> <name> <fields>; eq.set_parameter, eq.set_bypass, eq.reset_filter, eq.set_chain_bypass <0|1>, eq.set_chain_solo <0|1>, eq.reset <bankId>, eq.join_banks <count> <bankIds...>, eq.commit_hidden <bankId>, eq.set_link <bankId> <linkId|->, eq.select_bank, gain.*, compressor.*, saturator.*, analyzer.clear, analyzer.set_view <0|1> <spectrum|analysis>, fit.start <pointCount> <curveDb...>, fit.complete, fit.fail; bang publishes definitions, EQ, and DSP snapshots after initialization"
    };
    inlet<> persistenceIn{
        this,
        "(message) persistence: restore <dictionary> or persistence_ready"
    };

    outlet<> eventOut{
        this,
        "(anything) events/snapshots: event 1 host <eventId> <name> <fields>; snapshot 1 host eq ...; snapshot 1 host definitions ..."
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

    timer<timer_options::defer_delivery> persistenceDelivery{
        this,
        MIN_FUNCTION {
            if (ready && persistenceDirty) {
                persistenceDirty = false;
                PublishPersistence();
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
            PublishDefinitions();
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
            else {
                if (!EnsureInitializedState()) return {};
                PublishReadyState();
                PublishAllSnapshots();
                PublishPersistence();
            }
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
            const auto persisted = object ? persistence::PersistenceCodec::Deserialize(*object) : std::nullopt;
            auto state = persisted.value_or(persistence::PersistenceCodec::Defaults());
            if (!host.Restore(std::move(state.eq), state.processor, 0, state.instanceId)) {
                auto defaults = persistence::PersistenceCodec::Defaults();
                if (!host.Restore(std::move(defaults.eq), defaults.processor, 0, defaults.instanceId)) {
                    debugOut.send("error", "persistence_defaults_failed");
                    return {};
                }
            }
            if (ready) {
                PublishAllSnapshots();
            }
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
        }) {
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
        debugOut.send("error", "persistence_defaults_failed");
        return false;
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
            const auto isLinkCommand =
                std::holds_alternative<domain::SetEqBankLinkCommand>(decoded.command);
            host.Handle(decoded.command);
            if (isLinkCommand) {
                persistenceDirty = false;
                PublishPersistence();
            }
        }
        catch (const std::exception&) {
            debugOut.send("error", "host_command_failed");
        }
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
            PublishDefinitions();
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

    void PublishDefinitions() {
        eventOut.send(maxadapter::AtomAdapter::Write(
            messaging::SnapshotCodec::EncodeDefinitions(domain::FilterDefinitions())));
        eventOut.send(maxadapter::AtomAdapter::Write(
            messaging::SnapshotCodec::EncodeProcessorDefinitions()));
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

    void ScheduleStatePublication(const std::string& storeName) {
        if (!ready) return;

        if (storeName == "eq") eqSnapshotDirty = true;
        else processorSnapshotDirty = true;
        dspSnapshotDirty = true;
        if (!stateDeliveryScheduled) {
            stateDeliveryScheduled = true;
            stateDelivery.set();
        }

        persistenceDirty = true;
        persistenceDelivery.delay(PersistenceDebounceMilliseconds);
    }

    consolidator::host::DeviceHost host;
    static constexpr double PersistenceDebounceMilliseconds = 100.0;
    long nextEventId = 1;
    bool ready = false;
    bool stateDeliveryScheduled = false;
    bool eqSnapshotDirty = false;
    bool dspSnapshotDirty = false;
    bool processorSnapshotDirty = false;
    bool persistenceDirty = false;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorDeviceHost, consolidator.devicehost);
