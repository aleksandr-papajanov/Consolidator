#pragma once

#include <cstdint>
#include <variant>

#include "Core/Domain/Ids/InstanceId.h"
#include "Core/Domain/State/StateEntry.h"

namespace consolidator::core
{

using RequestId = std::uint64_t;

enum class StateOperation : std::uint8_t
{
    Read,
    Write
};

struct StateMessage
{
    RequestId requestId = 0;
    InstanceId responseInstanceId{0};
    StateRequestEntries entries;
    std::uint16_t responseIndex = 0;
    std::uint16_t responseCount = 1;
};

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
