#pragma once

#include "Ids/DomainIds.h"
#include "Results/FitResult.h"

#include <string>
#include <variant>

namespace consolidator::domain {

struct AttachComponentCommand {
    RequestId requestId{};
    ComponentId componentId{};
    std::string type;
    ProtocolVersion protocolVersion = 1;
};

struct DetachComponentCommand {
    RequestId requestId{};
    ComponentId componentId{};
};

struct SetEqParameterCommand {
    RequestId requestId{};
    BankId bankId{};
    FilterId filterId{};
    std::string parameter;
    double value = 0.0;
};

struct ResetEqFilterCommand {
    RequestId requestId{};
    BankId bankId{};
    FilterId filterId{};
};

struct SetEqBypassCommand {
    RequestId requestId{};
    BankId bankId{};
    FilterId filterId{};
    bool bypass = false;
};

struct AddEqBankCommand {
    RequestId requestId{};
    std::string name;
};

struct RemoveEqBankCommand {
    RequestId requestId{};
    BankId bankId{};
};

struct RenameEqBankCommand {
    RequestId requestId{};
    BankId bankId{};
    std::string name;
};

struct SelectEqBankCommand {
    RequestId requestId{};
    BankId bankId{};
};

struct ListenAnalyzerCommand {
    RequestId requestId{};
    bool enabled = false;
};

struct StartFitCommand {
    RequestId requestId{};
};

struct CancelFitCommand {
    RequestId requestId{};
    SessionId sessionId{};
};

struct ClearFitCommand {
    RequestId requestId{};
};

struct CompleteFitCommand {
    RequestId requestId{};
    FitResult result;
};

struct FailFitCommand {
    RequestId requestId{};
    SessionId sessionId{};
    std::string error;
};

using Command = std::variant<
    AttachComponentCommand,
    DetachComponentCommand,
    SetEqParameterCommand,
    SetEqBypassCommand,
    ResetEqFilterCommand,
    AddEqBankCommand,
    RemoveEqBankCommand,
    RenameEqBankCommand,
    SelectEqBankCommand,
    ListenAnalyzerCommand,
    StartFitCommand,
    CancelFitCommand,
    ClearFitCommand,
    CompleteFitCommand,
    FailFitCommand
>;

} // namespace consolidator::domain
