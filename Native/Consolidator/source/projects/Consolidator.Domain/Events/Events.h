#pragma once

#include "Ids/DomainIds.h"
#include "Operations/OperationTypes.h"
#include "States/States.h"

#include <string>
#include <cstddef>
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

struct ParameterUpdatedEvent {
    StoreRevision revision = 0;
    std::string device;
    long bankId = 0;
    long filterId = 0;
    std::string parameter;
    double value = 0.0;
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

enum class FitTargetKind {
    Residual,
    Absolute
};

struct FitRequestedEvent {
    SessionId sessionId{};
    BankId bankId{};
    FitTargetKind targetKind = FitTargetKind::Residual;
    std::vector<double> curveDb;
};

struct AnalyzerViewChangedEvent {
    bool visible = false;
    AnalyzerViewMode mode = AnalyzerViewMode::Spectrum;
};

using Event = std::variant<
    HostInitializedEvent,
    StoreUpdatedEvent,
    ParameterUpdatedEvent,
    CommandRejectedEvent,
    FitRequestedEvent,
    AnalyzerViewChangedEvent,
    OperationChangedEvent
>;

} // namespace consolidator::domain
