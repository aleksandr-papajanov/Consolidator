#pragma once

#include "Ids/DomainIds.h"
#include "Operations/OperationTypes.h"
#include "States/States.h"

#include <string>
#include <variant>
#include <vector>

namespace consolidator::domain {

struct HostInitializedEvent {
    StoreRevision revision = 0;
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

struct FitRequestedEvent {
    SessionId sessionId{};
    BankId bankId{};
    std::vector<double> curveDb;
};

struct AnalyzerViewChangedEvent {
    bool visible = false;
    AnalyzerViewMode mode = AnalyzerViewMode::Spectrum;
};

using Event = std::variant<
    HostInitializedEvent,
    StoreUpdatedEvent,
    CommandRejectedEvent,
    FitRequestedEvent,
    AnalyzerViewChangedEvent,
    OperationChangedEvent
>;

} // namespace consolidator::domain
