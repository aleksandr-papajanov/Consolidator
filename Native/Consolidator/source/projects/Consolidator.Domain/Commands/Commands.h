#pragma once

#include "Ids/DomainIds.h"
#include "Models/ProcessorState.h"
#include "Operations/OperationTypes.h"
#include "Results/FitResult.h"

#include <string>
#include <variant>
#include <vector>

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

struct SetEqChainBypassCommand {
    RequestId requestId{};
    BankId bankId{};
    bool bypass = false;
};

struct ResetEqChainCommand {
    RequestId requestId{};
    BankId bankId{};
};

struct AddEqBankCommand {
    RequestId requestId{};
    std::string name;
};

struct RemoveEqBankCommand {
    RequestId requestId{};
    BankId bankId{};
};

struct RemoveEqBanksCommand {
    RequestId requestId{};
    std::vector<BankId> bankIds;
};

struct SetEqBanksBypassCommand {
    RequestId requestId{};
    bool bypass = false;
    std::vector<BankId> bankIds;
};

struct SoloEqBanksCommand {
    RequestId requestId{};
    std::vector<BankId> bankIds;
};

struct JoinEqBanksCommand {
    RequestId requestId{};
    std::vector<BankId> bankIds;
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

struct SetCompressorModeCommand {
    RequestId requestId{};
    long mode = 0;
};

struct SetCompressorDetectorParameterCommand {
    RequestId requestId{};
    long filterId = 1;
    std::string parameter;
    double value = 0.0;
};

struct SetCompressorDetectorListenCommand {
    RequestId requestId{};
    long filterId = 0;
};

struct ResetCompressorCommand {
    RequestId requestId{};
};

struct SetSaturatorParameterCommand {
    RequestId requestId{};
    std::string parameter = "saturation";
    double value = 0.0;
};

struct SetSaturatorBypassCommand {
    RequestId requestId{};
    bool bypass = false;
};

struct SetSaturatorModeCommand {
    RequestId requestId{};
    long mode = 0;
};

struct SetSaturatorDetectorParameterCommand {
    RequestId requestId{};
    long filterId = 1;
    std::string parameter;
    double value = 0.0;
};

struct SetSaturatorDetectorListenCommand {
    RequestId requestId{};
    long filterId = 0;
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
    std::vector<double> curveDb;
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
    SetEqChainBypassCommand,
    ResetEqChainCommand,
    AddEqBankCommand,
    RemoveEqBankCommand,
    RemoveEqBanksCommand,
    SetEqBanksBypassCommand,
    SoloEqBanksCommand,
    JoinEqBanksCommand,
    RenameEqBankCommand,
    SelectEqBankCommand,
    SetGainParameterCommand,
    SetCompressorParameterCommand,
    SetCompressorBypassCommand,
    SetCompressorModeCommand,
    SetCompressorDetectorParameterCommand,
    SetCompressorDetectorListenCommand,
    ResetCompressorCommand,
    SetSaturatorParameterCommand,
    SetSaturatorBypassCommand,
    SetSaturatorModeCommand,
    SetSaturatorDetectorParameterCommand,
    SetSaturatorDetectorListenCommand,
    ResetSaturatorCommand,
    ListenAnalyzerCommand,
    StartFitCommand,
    CancelFitCommand,
    ClearFitCommand,
    CompleteFitCommand,
    FailFitCommand
>;

} // namespace consolidator::domain
