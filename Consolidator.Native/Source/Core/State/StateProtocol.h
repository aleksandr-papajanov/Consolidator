#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <variant>

#include "Core/Groups/GroupId.h"
#include "Core/Instance/InstanceId.h"
#include "Core/Parameters/DspIds.h"
#include "Core/Parameters/ParameterRoute.h"
#include "Core/Parameters/ParameterValue.h"

namespace consolidator::core
{

using RequestId = std::uint64_t;

struct StatePath
{
    std::optional<InstanceId> instanceId;
    std::optional<dsp::DeviceId> deviceId;
    std::array<dsp::RouteNodeId, 3> nodes{};
    std::size_t depth = 0;
    std::optional<dsp::ParameterId> parameterId;

    [[nodiscard]] bool Matches(const StatePath& candidate) const noexcept
    {
        if (instanceId && instanceId != candidate.instanceId) return false;
        if (deviceId && deviceId != candidate.deviceId) return false;
        if (parameterId && parameterId != candidate.parameterId) return false;
        if (depth > candidate.depth) return false;
        for (std::size_t index = 0; index < depth; ++index)
        {
            if (nodes[index] != candidate.nodes[index]) return false;
        }
        return true;
    }
};

using StateValue = std::variant<bool, std::int32_t, float, InstanceId, dsp::BankId, GroupId>;

struct StateEntry
{
    StatePath path;
    StateValue value;
};

inline StatePath ToStatePath(const dsp::ParameterRoute& route)
{
    StatePath path;
    path.deviceId = route.GetDeviceId();
    path.parameterId = route.GetParameterId();
    path.depth = route.GetDepth();
    for (std::size_t index = 0; index < path.depth; ++index)
    {
        path.nodes[index] = route.GetNode(index);
    }
    return path;
}

// A bounded response is required because the producer is the audio thread.
struct StateSnapshot
{
    static constexpr std::size_t kMaximumEntryCount = 256;

    std::array<StateEntry, kMaximumEntryCount> entries{};
    std::size_t size = 0;

    void Clear() noexcept { size = 0; }
    [[nodiscard]] bool TryAppend(StateEntry entry) noexcept
    {
        if (size == entries.size())
        {
            return false;
        }
        entries[size++] = std::move(entry);
        return true;
    }
};

} // namespace consolidator::core
