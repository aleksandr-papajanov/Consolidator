#pragma once

#include "Ids/DomainIds.h"
#include "Models/EqSection.h"
#include "Models/ProcessorState.h"
#include "Results/FitResult.h"

#include <string>
#include <variant>

namespace consolidator::domain {

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

struct SetEqSectionBypassCommand {
    RequestId requestId{};
    BankId bankId{};
    models::EqSection section = models::EqSection::Pre;
    bool bypass = false;
};

struct ResetEqSectionCommand {
    RequestId requestId{};
    BankId bankId{};
    models::EqSection section = models::EqSection::Pre;
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

struct SetGainParameterCommand {
    RequestId requestId{};
    GainStage stage = GainStage::Input;
    double gainDb = 0.0;
};

struct SetCompressorParameterCommand {
    RequestId requestId{};
    std::string parameter;
    double value = 0.0;
};

struct SetCompressorBypassCommand {
    RequestId requestId{};
    bool bypass = false;
};

struct ResetCompressorCommand {
    RequestId requestId{};
};

struct SetSaturatorParameterCommand {
    RequestId requestId{};
    double saturation = 0.0;
};

struct SetSaturatorBypassCommand {
    RequestId requestId{};
    bool bypass = false;
};

struct ResetSaturatorCommand {
    RequestId requestId{};
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
    SetEqParameterCommand,
    SetEqBypassCommand,
    ResetEqFilterCommand,
    SetEqSectionBypassCommand,
    ResetEqSectionCommand,
    AddEqBankCommand,
    RemoveEqBankCommand,
    RenameEqBankCommand,
    SelectEqBankCommand,
    SetGainParameterCommand,
    SetCompressorParameterCommand,
    SetCompressorBypassCommand,
    ResetCompressorCommand,
    SetSaturatorParameterCommand,
    SetSaturatorBypassCommand,
    ResetSaturatorCommand,
    ListenAnalyzerCommand,
    StartFitCommand,
    CancelFitCommand,
    ClearFitCommand,
    CompleteFitCommand,
    FailFitCommand
>;

} // namespace consolidator::domain
