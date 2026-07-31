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
            else if constexpr (std::is_same_v<Event, domain::StoreUpdatedEvent>) {
                writer.Write(std::string{ "store.updated" }).Write(value.storeName)
                    .Write(static_cast<std::int64_t>(value.revision))
                    .Write(static_cast<std::int64_t>(value.requestId.value));
            }
            else if constexpr (std::is_same_v<Event, domain::ParameterUpdatedEvent>) {
                writer.Write(std::string{ "parameter.updated" })
                    .Write(static_cast<std::int64_t>(value.revision)).Write(value.device)
                    .Write(static_cast<std::int64_t>(value.bankId))
                    .Write(static_cast<std::int64_t>(value.filterId))
                    .Write(value.parameter).Write(value.value);
            }
            else if constexpr (std::is_same_v<Event, domain::CommandRejectedEvent>) {
                writer.Write(std::string{ "command.rejected" })
                    .Write(static_cast<std::int64_t>(value.requestId.value)).Write(value.code);
            }
            else if constexpr (std::is_same_v<Event, domain::FitRequestedEvent>) {
                writer.Write(std::string{ "fit.requested" })
                    .Write(static_cast<std::int64_t>(value.sessionId.value))
                    .Write(static_cast<std::int64_t>(value.bankId.value))
                    .Write(std::string{
                        value.targetKind == domain::FitTargetKind::Absolute ? "absolute" : "residual"
                    })
                    .Write(static_cast<std::int64_t>(value.curveDb.size()));
                for (const auto valueDb : value.curveDb) writer.Write(valueDb);
            }
            else if constexpr (std::is_same_v<Event, domain::AnalyzerViewChangedEvent>) {
                writer.Write(std::string{ "analyzer.view_changed" })
                    .Write(value.visible)
                    .Write(std::string{
                        value.mode == domain::AnalyzerViewMode::Spectrum ? "spectrum" : "analysis"
                    });
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
        if (*name == "parameter.updated") {
            const auto revision = reader.ReadInt();
            const auto device = reader.ReadString();
            const auto bankId = reader.ReadInt();
            const auto filterId = reader.ReadInt();
            const auto parameter = reader.ReadString();
            const auto value = reader.ReadDouble();
            if (!revision || !device || !bankId || !filterId || !parameter || !value ||
                *revision < 0 ||
                device->empty() || parameter->empty() || !std::isfinite(*value) ||
                !reader.RequireEnd()) {
                return Invalid("invalid_parameter_updated", reader.Index());
            }
            return Success(domain::ParameterUpdatedEvent{
                static_cast<domain::StoreRevision>(*revision), *device,
                static_cast<long>(*bankId), static_cast<long>(*filterId),
                *parameter, *value
            }, *eventId);
        }
        if (*name == "command.rejected") {
            const auto requestId = reader.ReadInt();
            const auto code = reader.ReadString();
            if (!requestId || !code || *requestId < 1 || code->empty() || !reader.RequireEnd()) return Invalid("invalid_command_rejected", reader.Index());
            return Success(domain::CommandRejectedEvent{ { *requestId }, *code }, *eventId);
        }
        if (*name == "fit.requested") {
            const auto sessionId = reader.ReadInt();
            const auto bankId = reader.ReadInt();
            const auto targetKind = reader.ReadString();
            const auto pointCount = reader.ReadInt();
            if (!sessionId || !bankId || !targetKind || !pointCount || *sessionId < 1 || *bankId < 1 ||
                *pointCount < 2 || *pointCount > 4096) {
                return Invalid("invalid_fit_requested", reader.Index());
            }
            std::optional<domain::FitTargetKind> kind;
            if (*targetKind == "absolute") kind = domain::FitTargetKind::Absolute;
            else if (*targetKind == "residual") kind = domain::FitTargetKind::Residual;
            else return Invalid("invalid_fit_requested", reader.Index());
            std::vector<double> curveDb;
            curveDb.reserve(static_cast<std::size_t>(*pointCount));
            for (long index = 0; index < *pointCount; ++index) {
                const auto value = reader.ReadDouble();
                if (!value) return Invalid("invalid_fit_requested", reader.Index());
                curveDb.push_back(*value);
            }
            if (!reader.RequireEnd()) return Invalid("invalid_fit_requested", reader.Index());
            return Success(domain::FitRequestedEvent{
                { *sessionId }, { *bankId }, *kind, std::move(curveDb)
            }, *eventId);
        }
        if (*name == "analyzer.view_changed") {
            const auto visible = reader.ReadBool();
            const auto mode = reader.ReadString();
            if (!visible || !mode || !reader.RequireEnd()) {
                return Invalid("invalid_analyzer_view_changed", reader.Index());
            }
            if (*mode == "spectrum") {
                return Success(domain::AnalyzerViewChangedEvent{
                    *visible, domain::AnalyzerViewMode::Spectrum
                }, *eventId);
            }
            if (*mode == "analysis") {
                return Success(domain::AnalyzerViewChangedEvent{
                    *visible, domain::AnalyzerViewMode::Analysis
                }, *eventId);
            }
            return Invalid("invalid_analyzer_view_mode", reader.Index());
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
