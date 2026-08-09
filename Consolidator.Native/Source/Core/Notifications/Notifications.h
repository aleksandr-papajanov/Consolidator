#pragma once

#include <variant>

#include "Core/State/StateProtocol.h"

namespace consolidator::core
{

struct StateResponse
{
    RequestId requestId;
    InstanceId responseInstanceId{0};
    InstanceId appliedInstanceId{0};
    StateOperation operation;
    StateResponseEntries entries;
    std::uint16_t responseIndex{0};
    std::uint16_t responseCount{1};
    bool isFinal{true};
    bool truncated{false};
};

// ---- Notifications: reactions to commands, produced by the system ----

struct ParameterStateNotification
{
    StatePath path;
    dsp::ParameterValue value;
    dsp::ParameterValue minimum;
    dsp::ParameterValue maximum;
};

struct FitCompletedNotification
{
};

struct FitFailedNotification
{
    std::string error;
};

struct FitCancelledNotification
{
};

struct AnalyzerClearedNotification
{
};

struct UndoCompletedNotification
{
};

struct RedoCompletedNotification
{
};

struct HistoryOperationRestoredNotification
{
    std::string operationId;
    bool isUndo = false;
};

using Notification = std::variant<
    ParameterStateNotification,
    FitCompletedNotification,
    FitFailedNotification,
    FitCancelledNotification,
    AnalyzerClearedNotification,
    UndoCompletedNotification,
    RedoCompletedNotification,
    HistoryOperationRestoredNotification
>;

} // namespace consolidator::core
