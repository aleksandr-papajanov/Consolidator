#pragma once

#include <string>
#include <variant>

#include "Core/State/StateProtocol.h"

namespace consolidator::core
{

struct StateResponse
{
    RequestId requestId;
    InstanceId responseInstanceId{0};
    StateOperation operation;
    StateResponseEntries entries;
};

// ---- Notifications: reactions to commands, produced by the system ----

struct ParameterUpdatedNotification
{
    std::string device;
    std::string parameter;
    double value = 0.0;
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
    ParameterUpdatedNotification,
    FitCompletedNotification,
    FitFailedNotification,
    FitCancelledNotification,
    AnalyzerClearedNotification,
    UndoCompletedNotification,
    RedoCompletedNotification,
    HistoryOperationRestoredNotification
>;

} // namespace consolidator::core
