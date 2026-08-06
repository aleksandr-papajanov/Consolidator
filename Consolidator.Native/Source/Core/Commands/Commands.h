#pragma once

#include <string>
#include <variant>
#include <vector>

#include "Core/Groups/GroupId.h"
#include "Dsp/Parameters/ParameterAddress.h"
#include "Dsp/Parameters/ParameterValue.h"

namespace consolidator::core
{

// ---- Forward declarations / placeholders for yet-unimplemented types ----

enum class AnalyzerViewMode
{
    Spectrum,
    Curve
};

struct FitResult
{
    // Placeholder
};

using SessionId = std::string;

// ---- DSP: single unified parameter change ----
// Covers ALL DSP parameter changes including:
//   - EQ: filter frequency/Q/gain/bypass/type, chain bypass/solo/reset
//   - Gain: input/output gain dB, bypass
//   - Compressor: threshold/ratio/attack/release/output/mix/bypass/reset
//   - Compressor detector: frequency/Q/gain
//   - Saturator: drive/output/mix/bypass/reset
//   - Saturator detector: frequency/Q/gain
//
// Every possible change is addressed via ParameterAddress (DeviceKind,
// ElementId, ParameterId) + ParameterValue.

struct DspParameterChangeCommand
{
    dsp::ParameterAddress address;
    dsp::ParameterValue value;
};

// ---- Group commands ----

struct JoinGroupsCommand
{
    std::vector<GroupId> groupIds;
};

struct CommitGroupCommand
{
    GroupId groupId;
};

struct CommitAllGroupsCommand
{
};

struct SelectGroupCommand
{
    GroupId groupId;
};

struct SetBankLinkCommand
{
    GroupId bankId;
    std::string linkId;
};

struct ResetGroupCommand
{
    GroupId groupId;
};

struct ResetAllGroupsCommand
{
};

// ---- Analyzer commands ----

struct ClearAnalyzerCommand
{
};

struct SetAnalyzerViewCommand
{
    bool visible = false;
    AnalyzerViewMode mode = AnalyzerViewMode::Spectrum;
};

// ---- History commands ----

struct BeginHistoryCommand
{
    std::string operationId;
};

struct EndHistoryCommand
{
    std::string operationId;
};

struct UndoCommand
{
};

struct RedoCommand
{
};

struct RestoreHistoryOperationCommand
{
    std::string operationId;
    bool isUndo = false;
};

// ---- Fit commands ----

struct StartFitCommand
{
    std::vector<double> curveDb;
};

struct CancelFitCommand
{
    SessionId sessionId;
};

struct ClearFitCommand
{
};

struct CompleteFitCommand
{
    FitResult result;
};

struct FailFitCommand
{
    SessionId sessionId;
    std::string error;
};

// ---- Command variant ----

using Command = std::variant<
    DspParameterChangeCommand,
    JoinGroupsCommand,
    CommitGroupCommand,
    CommitAllGroupsCommand,
    SelectGroupCommand,
    SetBankLinkCommand,
    ResetGroupCommand,
    ResetAllGroupsCommand,
    ClearAnalyzerCommand,
    SetAnalyzerViewCommand,
    StartFitCommand,
    CancelFitCommand,
    ClearFitCommand,
    CompleteFitCommand,
    FailFitCommand,
    BeginHistoryCommand,
    EndHistoryCommand,
    UndoCommand,
    RedoCommand,
    RestoreHistoryOperationCommand
>;

} // namespace consolidator::core