#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>

#include "Core/Groups/GroupId.h"
#include "Core/Instance/InstanceId.h"
#include "Core/Parameters/DspIds.h"
#include "Core/Parameters/ParameterValue.h"

namespace consolidator::core
{

using RequestId = std::uint64_t;

enum class StateField : std::uint8_t
{
    InstanceId,
    SelectedBank,
    BankId,
    GroupId,
    DspParameter
};

struct StatePath
{
    constexpr StatePath() noexcept = default;

    [[nodiscard]] static constexpr StatePath Instance(InstanceId instance) noexcept
    {
        StatePath path;
        path.instanceId = instance;
        return path;
    }

    [[nodiscard]] static constexpr StatePath SelectedBank(InstanceId instance) noexcept
    {
        auto path = Instance(instance);
        path.field = StateField::SelectedBank;
        return path;
    }

    [[nodiscard]] static constexpr StatePath BankGroup(
        InstanceId instance,
        dsp::BankId bank) noexcept
    {
        auto path = Instance(instance);
        path.field = StateField::GroupId;
        path.nodes[0] = static_cast<dsp::RouteNodeId>(
            static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) +
            dsp::detail::ToIndex(bank));
        path.depth = 1;
        return path;
    }

    template <typename Route>
    [[nodiscard]] static StatePath DspParameter(
        InstanceId instance,
        const Route& route)
    {
        StatePath path;
        path.deviceId = route.GetDeviceId();
        path.parameterId = route.GetParameterId();
        path.depth = route.GetDepth();
        for (std::size_t index = 0; index < path.depth; ++index)
        {
            path.nodes[index] = route.GetNode(index);
        }
        path.field = StateField::DspParameter;
        path.instanceId = instance;
        return path;
    }

    constexpr StatePath(
        dsp::DeviceId device,
        dsp::ParameterId parameter) noexcept
        : deviceId(device)
        , parameterId(parameter)
    {
    }

    template <typename... NodeIds>
    constexpr StatePath(
        dsp::DeviceId device,
        dsp::ParameterId parameter,
        NodeIds... nodeIds) noexcept
        : deviceId(device)
        , parameterId(parameter)
        , nodes{static_cast<dsp::RouteNodeId>(nodeIds)...}
        , depth(sizeof...(nodeIds))
    {
        static_assert(sizeof...(NodeIds) <= 3);
    }

    [[nodiscard]] constexpr dsp::DeviceId GetDeviceId() const noexcept
    {
        assert(deviceId.has_value());
        return *deviceId;
    }
    [[nodiscard]] constexpr dsp::ParameterId GetParameterId() const noexcept
    {
        assert(parameterId.has_value());
        return *parameterId;
    }
    [[nodiscard]] constexpr std::size_t GetDepth() const noexcept { return depth; }
    [[nodiscard]] constexpr dsp::RouteNodeId GetNode(std::size_t index) const noexcept { return nodes[index]; }

    [[nodiscard]] constexpr StatePath WithParameter(dsp::ParameterId parameter) const noexcept
    {
        auto result = *this;
        result.parameterId = parameter;
        return result;
    }

    [[nodiscard]] constexpr StatePath WithNode(dsp::RouteNodeId node) const noexcept
    {
        auto result = *this;
        assert(result.depth < result.nodes.size());
        result.nodes[result.depth++] = node;
        return result;
    }

    std::optional<InstanceId> instanceId;
    std::optional<StateField> field;
    std::optional<dsp::DeviceId> deviceId;
    std::optional<dsp::ParameterId> parameterId;
    std::array<dsp::RouteNodeId, 3> nodes{};
    std::size_t depth = 0;

    [[nodiscard]] bool Matches(const StatePath& candidate) const noexcept
    {
        if (instanceId && instanceId != candidate.instanceId) return false;
        if (field && field != candidate.field) return false;
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

using StateValue = std::variant<std::monostate, bool, std::int32_t, float, InstanceId, dsp::BankId, GroupId>;

struct StateEntry
{
    StatePath path;
    StateValue value;
};

template <typename Route>
inline StatePath ToStatePath(const Route& route)
{
    StatePath path;
    path.deviceId = route.GetDeviceId();
    path.parameterId = route.GetParameterId();
    path.depth = route.GetDepth();
    for (std::size_t index = 0; index < path.depth; ++index)
    {
        path.nodes[index] = route.GetNode(index);
    }
    path.field = StateField::DspParameter;
    return path;
}

template <std::size_t Capacity>
struct FixedStateList
{
    std::array<StateEntry, Capacity> entries{};
    std::size_t size = 0;
    bool truncated = false;

    void Clear() noexcept
    {
        size = 0;
        truncated = false;
    }
    [[nodiscard]] bool TryAppend(StateEntry entry) noexcept
    {
        if (size == entries.size())
        {
            truncated = true;
            return false;
        }
        entries[size++] = std::move(entry);
        return true;
    }
};

using StateRequestEntries = FixedStateList<16>;
using StateResponseEntries = FixedStateList<256>;
using StateSnapshot = StateResponseEntries;

enum class StateOperation : std::uint8_t
{
    Read,
    Write
};

enum class StateWriteStatus : std::uint8_t
{
    NotHandled,
    Applied,
    Unchanged,
    Rejected
};

struct StateMessage
{
    RequestId requestId = 0;
    InstanceId responseInstanceId{0};
    StateRequestEntries entries;
    std::uint16_t responseIndex = 0;
    std::uint16_t responseCount = 1;
};

} // namespace consolidator::core
