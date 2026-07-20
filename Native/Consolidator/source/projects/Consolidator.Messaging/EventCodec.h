#pragma once

#include "AtomReader.h"
#include "AtomWriter.h"
#include "Events/Events.h"

#include <cmath>
#include <limits>
#include <optional>
#include <type_traits>
#include <utility>

namespace consolidator::messaging {

struct DecodedEvent {
    domain::Event event;
    domain::EventId eventId{};
    ProtocolError error;

    bool Succeeded() const noexcept { return error.code.empty(); }
};

class EventCodec final {
public:
    template <typename Sender>
    static void Send(const domain::Event& event, domain::EventId eventId, Sender&& sender) {
        std::visit([eventId, &sender](const auto& value) {
            using Event = std::decay_t<decltype(value)>;
            AtomWriter writer;
            writer.Write(std::string{ "event" }).Write(static_cast<std::int64_t>(1))
                .Write(std::string{ "host" }).Write(static_cast<std::int64_t>(eventId.value));
            if constexpr (std::is_same_v<Event, domain::HostInitializedEvent>) {
                writer.Write(std::string{ "host.initialized" }).Write(static_cast<std::int64_t>(value.revision));
            }
            else if constexpr (std::is_same_v<Event, domain::ComponentAttachedEvent>) {
                writer.Write(std::string{ "component.attached" })
                    .Write(static_cast<std::int64_t>(value.componentId.value)).Write(value.type);
            }
            else if constexpr (std::is_same_v<Event, domain::StoreUpdatedEvent>) {
                writer.Write(std::string{ "store.updated" }).Write(value.storeName)
                    .Write(static_cast<std::int64_t>(value.revision))
                    .Write(static_cast<std::int64_t>(value.requestId.value));
            }
            else if constexpr (std::is_same_v<Event, domain::CommandRejectedEvent>) {
                writer.Write(std::string{ "command.rejected" })
                    .Write(static_cast<std::int64_t>(value.requestId.value)).Write(value.code);
            }
            else if constexpr (std::is_same_v<Event, domain::OperationChangedEvent>) {
                writer.Write(std::string{ "operation.changed" }).Write(value.operation)
                    .Write(static_cast<std::int64_t>(value.sessionId.value))
                    .Write(static_cast<std::int64_t>(value.status))
                    .Write(value.progress).Write(value.error);
            }
            sender(std::move(writer).Finish());
        }, event);
    }

    static DecodedEvent Decode(const AtomList& atoms) {
        AtomReader reader(atoms);
        const auto category = reader.ReadString();
        const auto version = reader.ReadInt();
        const auto source = reader.ReadString();
        const auto eventId = reader.ReadInt();
        const auto name = reader.ReadString();
        if (!category || !version || !source || !eventId || !name ||
            *category != "event" || *version != 1 || *source != "host" || *eventId < 1) {
            return Invalid("invalid_event_header", reader.Index());
        }

        if (*name == "host.initialized") {
            const auto revision = reader.ReadInt();
            if (!revision || *revision < 0 || !reader.RequireEnd()) return Invalid("invalid_host_initialized", reader.Index());
            return Success(domain::HostInitializedEvent{ static_cast<domain::StoreRevision>(*revision) }, *eventId);
        }
        if (*name == "component.attached") {
            const auto componentId = reader.ReadInt();
            const auto type = reader.ReadString();
            if (!componentId || !type || *componentId < 1 || type->empty() || !reader.RequireEnd()) return Invalid("invalid_component_attached", reader.Index());
            return Success(domain::ComponentAttachedEvent{ { *componentId }, *type }, *eventId);
        }
        if (*name == "store.updated") {
            const auto storeName = reader.ReadString();
            const auto revision = reader.ReadInt();
            const auto requestId = reader.ReadInt();
            if (!storeName || !revision || !requestId || storeName->empty() || *revision < 0 ||
                *requestId < 1 || !reader.RequireEnd()) return Invalid("invalid_store_updated", reader.Index());
            return Success(domain::StoreUpdatedEvent{
                *storeName, static_cast<domain::StoreRevision>(*revision), { *requestId }
            }, *eventId);
        }
        if (*name == "command.rejected") {
            const auto requestId = reader.ReadInt();
            const auto code = reader.ReadString();
            if (!requestId || !code || *requestId < 1 || code->empty() || !reader.RequireEnd()) return Invalid("invalid_command_rejected", reader.Index());
            return Success(domain::CommandRejectedEvent{ { *requestId }, *code }, *eventId);
        }
        if (*name == "operation.changed") {
            const auto operation = reader.ReadString();
            const auto sessionId = reader.ReadInt();
            const auto status = reader.ReadInt();
            const auto progress = reader.ReadDouble();
            const auto error = reader.ReadString();
            if (!operation || !sessionId || !status || !progress || !error || operation->empty() ||
                *sessionId < 0 || *sessionId > std::numeric_limits<long>::max() ||
                !std::isfinite(*progress) || *progress < 0.0 || *progress > 1.0 ||
                !reader.RequireEnd() ||
                *status < static_cast<std::int64_t>(domain::OperationStatus::Idle) ||
                *status > static_cast<std::int64_t>(domain::OperationStatus::Failed)) {
                return Invalid("invalid_operation_changed", reader.Index());
            }
            return Success(domain::OperationChangedEvent{
                *operation, { *sessionId }, static_cast<domain::OperationStatus>(*status), *progress, *error
            }, *eventId);
        }
        return Invalid("unknown_event", reader.Index());
    }

private:
    static DecodedEvent Invalid(std::string code, std::size_t index) {
        return { domain::HostInitializedEvent{}, {}, { std::move(code), index, "valid event", "invalid" } };
    }

    template <typename Event>
    static DecodedEvent Success(Event event, std::int64_t eventId) {
        return { std::move(event), { eventId }, {} };
    }
};

} // namespace consolidator::messaging
