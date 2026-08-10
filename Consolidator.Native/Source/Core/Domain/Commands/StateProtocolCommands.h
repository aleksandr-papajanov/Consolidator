#pragma once

#include <variant>

#include "Core/Domain/Ids/InstanceId.h"
#include "Core/Domain/State/StateProtocol.h"

namespace consolidator::core
{

struct StateCommand
{
    StateOperation operation;
    StateMessage message;
};

using Command = std::variant<StateCommand>;

struct InstanceCommand
{
    InstanceId sourceInstanceId;
    Command command;
};

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

} // namespace consolidator::core
