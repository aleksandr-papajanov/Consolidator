#pragma once

#include "Ids/DomainIds.h"
#include "Models/ProcessorState.h"
#include "Operations/OperationTypes.h"
#include "Results/FitResult.h"
#include "States/States.h"

#include <cstddef>
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

struct SetEqParameterIndexCommand {
    RequestId requestId{};
    BankId bankId{};
    FilterId filterId{};
    std::size_t parameterIndex = 0;
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
    bool bypass = false;
};

struct SetEqChainSoloCommand {
    RequestId requestId{};
    bool solo = false;
};

struct ResetEqChainCommand {
    RequestId requestId{};
    BankId bankId{};
};

struct JoinEqBanksCommand {
    RequestId requestId{};
    std::vector<BankId> bankIds;
};

struct CommitHiddenEqBankCommand {
    RequestId requestId{};
    BankId bankId{};
};

struct SetEqBankLinkCommand {
    RequestId requestId{};
    BankId bankId{};
    std::string linkId;
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

struct ClearAnalyzerCommand {
    RequestId requestId{};
};

struct SetAnalyzerViewCommand {
    RequestId requestId{};
    bool visible = false;
    AnalyzerViewMode mode = AnalyzerViewMode::Spectrum;
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
    SetEqParameterIndexCommand,
    SetEqBypassCommand,
    ResetEqFilterCommand,
    SetEqChainBypassCommand,
    SetEqChainSoloCommand,
    ResetEqChainCommand,
    JoinEqBanksCommand,
    CommitHiddenEqBankCommand,
    SetEqBankLinkCommand,
    SelectEqBankCommand,
    SetGainParameterCommand,
    SetCompressorParameterCommand,
    SetCompressorBypassCommand,
    SetCompressorDetectorParameterCommand,
    SetCompressorDetectorListenCommand,
    ResetCompressorCommand,
    SetSaturatorParameterCommand,
    SetSaturatorBypassCommand,
    SetSaturatorDetectorParameterCommand,
    SetSaturatorDetectorListenCommand,
    ResetSaturatorCommand,
    ClearAnalyzerCommand,
    SetAnalyzerViewCommand,
    StartFitCommand,
    CancelFitCommand,
    ClearFitCommand,
    CompleteFitCommand,
    FailFitCommand
>;

} // namespace consolidator::domain
