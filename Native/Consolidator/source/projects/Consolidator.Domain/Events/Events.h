#pragma once

#include "Ids/DomainIds.h"
#include "Operations/OperationTypes.h"

#include <string>
#include <variant>

namespace consolidator::domain {

struct HostInitializedEvent {
    StoreRevision revision = 0;
};

struct ComponentAttachedEvent {
    ComponentId componentId{};
    std::string type;
};

struct StoreUpdatedEvent {
    std::string storeName;
    StoreRevision revision = 0;
    RequestId requestId{};
};

struct CommandRejectedEvent {
    RequestId requestId{};
    std::string code;
};

struct OperationChangedEvent {
    std::string operation;
    SessionId sessionId{};
    OperationStatus status = OperationStatus::Idle;
    double progress = 0.0;
    std::string error;
};

using Event = std::variant<
    HostInitializedEvent,
    ComponentAttachedEvent,
    StoreUpdatedEvent,
    CommandRejectedEvent,
    OperationChangedEvent
>;

} // namespace consolidator::domain
