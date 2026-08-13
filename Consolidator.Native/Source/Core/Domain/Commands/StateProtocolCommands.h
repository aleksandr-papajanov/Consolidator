#pragma once

#include <cstdint>
#include <variant>

#include "Core/Domain/Ids/InstanceId.h"
#include "Core/Domain/State/StateEntry.h"
#include "Core/Registry/RegistrySnapshot.h"

namespace consolidator::core
{

using RequestId = std::uint64_t;

// Bounded state read/write protocol messages and their response envelope.
struct ReadStateCommand
{
    RequestId requestId{0};
    InstanceId instanceId{0};
    StateRequestEntries queries;
};

struct WriteStateCommand
{
    RequestId requestId{0};
    InstanceId instanceId{0};
    StateRequestEntries entries;
};

// Resets the selected DSP route's real-time memory on the next audio block.
struct ResetDspCommand
{
    RequestId requestId{0};
    InstanceId instanceId{0};
    StatePath target;
};

struct ReadRegistryCommand
{
    RequestId requestId{0};
    InstanceId instanceId{0};
};

using Command = std::variant<
    ReadStateCommand,
    WriteStateCommand,
    ResetDspCommand,
    ReadRegistryCommand>;

struct StateResponse
{
    RequestId requestId{0};
    InstanceId instanceId{0};
    StateResponseEntries entries;
    bool truncated{false};
};

enum class ActionStatus : std::uint8_t
{
    Accepted,
    Rejected
};

struct ActionResponse
{
    RequestId requestId{0};
    InstanceId instanceId{0};
    ActionStatus status{ActionStatus::Rejected};
};

struct RegistryResponse
{
    RequestId requestId{0};
    InstanceId instanceId{0};
    RegistrySnapshot snapshot;
};

using CommandResponse = std::variant<
    StateResponse,
    ActionResponse,
    RegistryResponse>;

} // namespace consolidator::core
