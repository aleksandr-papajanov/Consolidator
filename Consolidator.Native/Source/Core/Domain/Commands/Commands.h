#pragma once

// This file intentionally contains only disabled placeholders for command and
// notification scopes that are not implemented or used by the current core.
// Active protocol types live in dedicated headers:
//   StateProtocolCommands.h

namespace consolidator::core
{

// ---- Group commands: not implemented ----
// struct JoinGroupsCommand { std::vector<GroupId> groupIds; };
// struct CommitGroupCommand { GroupId groupId; };
// struct CommitAllGroupsCommand {};
// struct SelectGroupCommand { GroupId groupId; };
// struct SetBankLinkCommand { GroupId bankId; std::string linkId; };
// struct ResetGroupCommand { GroupId groupId; };
// struct ResetAllGroupsCommand {};

// ---- Analyzer commands: not implemented ----
// enum class AnalyzerViewMode { Spectrum, Curve };
// struct ClearAnalyzerCommand {};
// struct SetAnalyzerViewCommand
// {
//     bool visible = false;
//     AnalyzerViewMode mode = AnalyzerViewMode::Spectrum;
// };

// ---- History commands: not implemented ----
// struct BeginHistoryCommand { std::string operationId; };
// struct EndHistoryCommand { std::string operationId; };
// struct UndoCommand {};
// struct RedoCommand {};
// struct RestoreHistoryOperationCommand
// {
//     std::string operationId;
//     bool isUndo = false;
// };

// ---- Fit commands: not implemented ----
// struct FitResult {};
// struct StartFitCommand { std::vector<double> curveDb; };
// struct CancelFitCommand { std::string sessionId; };
// struct ClearFitCommand {};
// struct CompleteFitCommand { FitResult result; };
// struct FailFitCommand
// {
//     std::string sessionId;
//     std::string error;
// };

// ---- Non-parameter DSP commands: not implemented ----

// struct ResetSaturator {};
// struct ResetCompressor {};
// struct ResetEqualizer {};
// struct ResetEqFilter { dsp::FilterId filterId; };
// struct ResetGain { dsp::DeviceId gainId; };
// using DspAction = std::variant<
//     ResetSaturator,
//     ResetCompressor,
//     ResetEqualizer,
//     ResetEqFilter,
//     ResetGain
// >;

// ---- Notifications: not implemented ----
// struct ParameterStateNotification
// {
//     StatePath path;
//     dsp::ParameterVariant value;
//     dsp::ParameterVariant minimum;
//     dsp::ParameterVariant maximum;
// };
// struct FitCompletedNotification {};
// struct FitFailedNotification { std::string error; };
// struct FitCancelledNotification {};
// struct AnalyzerClearedNotification {};
// struct UndoCompletedNotification {};
// struct RedoCompletedNotification {};
// struct HistoryOperationRestoredNotification
// {
//     std::string operationId;
//     bool isUndo = false;
// };

} // namespace consolidator::core
