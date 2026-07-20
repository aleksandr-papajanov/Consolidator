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
        "(message) commands: command 1 <source> <requestId> <name> <fields>; component.attach, component.detach, eq.set_parameter, eq.set_bypass, eq.reset_filter, eq.add_bank, eq.remove_bank, eq.rename_bank, eq.select_bank, analyzer.listen, fit.start, fit.cancel, fit.clear, fit.complete, fit.fail; bang publishes definitions and EQ snapshots after initialization"
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
            PublishSnapshot();
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
                PublishReadyState();
                PublishSnapshot();
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
            if (!host.RestoreEq(std::move(state.eq), 0)) {
                auto defaults = persistence::PersistenceCodec::Defaults();
                if (!host.RestoreEq(std::move(defaults.eq), 0)) {
                    debugOut.send("error", "persistence_defaults_failed");
                    return {};
                }
            }
            if (ready) PublishSnapshot();
            return {};
        }
    };

    ConsolidatorDeviceHost()
        : host([this](const domain::Event& event) {
            if (const auto* rejected = std::get_if<domain::CommandRejectedEvent>(&event)) {
                debugOut.send("error", rejected->code);
            }
            messaging::EventCodec::Send(event, { nextEventId++ }, [this](const messaging::AtomList& atoms) {
                eventOut.send(maxadapter::AtomAdapter::Write(atoms));
            });
            if (ready && std::holds_alternative<domain::ComponentAttachedEvent>(event)) {
                PublishDefinitions();
                PublishSnapshot();
            }
            else if (std::holds_alternative<domain::StoreUpdatedEvent>(event)) {
                PublishSnapshot();
            }
            if (ready && std::holds_alternative<domain::StoreUpdatedEvent>(event)) PublishPersistence();
        }) {
        statusOut.send("status", "initializing");
    }

private:
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
        const auto lifecycleCommand =
            std::holds_alternative<domain::AttachComponentCommand>(decoded.command) ||
            std::holds_alternative<domain::DetachComponentCommand>(decoded.command);
        if (!ready && !lifecycleCommand) {
            debugOut.send("error", "host_not_ready");
            return;
        }
        try {
            host.Handle(decoded.command);
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

    void PublishSnapshot() {
        eventOut.send(maxadapter::AtomAdapter::Write(
            messaging::SnapshotCodec::EncodeEq(host.Eq().State(), host.Revision())));
    }

    void PublishDefinitions() {
        eventOut.send(maxadapter::AtomAdapter::Write(
            messaging::SnapshotCodec::EncodeDefinitions(domain::FilterDefinitions())));
    }

    void PublishPersistence() {
        persistence::PersistedDeviceState persisted{ persistence::PersistenceCodec::SchemaVersion, host.Eq().State() };
        maxadapter::MaxDictionarySerializer::Serialize(
            persistence::PersistenceCodec::Serialize(persisted),
            [this](const c74::min::atom& dictionary) {
                persistenceOut.send("dictionary", dictionary);
            });
    }

    consolidator::host::DeviceHost host;
    long nextEventId = 1;
    bool ready = false;
};

MIN_EXTERNAL_CUSTOM(ConsolidatorDeviceHost, consolidator.devicehost);
