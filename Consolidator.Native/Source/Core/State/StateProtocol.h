#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>

#include "Core/Groups/GroupId.h"
#include "Core/Instance/InstanceId.h"
#include "Core/Parameters/DspIds.h"
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

    [[nodiscard]] constexpr dsp::DeviceId GetDeviceId() const noexcept { return *deviceId; }
    [[nodiscard]] constexpr dsp::ParameterId GetParameterId() const noexcept { return *parameterId; }
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
        result.nodes[result.depth++] = node;
        return result;
    }

    std::optional<InstanceId> instanceId;
    std::optional<StateField> field;
    std::optional<dsp::DeviceId> deviceId;
    std::optional<dsp::ParameterId> parameterId;
    std::array<dsp::RouteNodeId, 3> nodes{};
    std::size_t depth = 0;
    std::optional<dsp::RouteNodeId> bankNode;

    [[nodiscard]] bool Matches(const StatePath& candidate) const noexcept
    {
        if (instanceId && instanceId != candidate.instanceId) return false;
        if (field && field != candidate.field) return false;
        if (bankNode && bankNode != candidate.bankNode) return false;
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

    void Clear() noexcept
    {
        size = 0;
    }
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

using StateRequestEntries = FixedStateList<16>;
using StateResponseEntries = FixedStateList<256>;
using StateSnapshot = StateResponseEntries;

enum class StateOperation : std::uint8_t
{
    Read,
    Write
};

struct StateMessage
{
    RequestId requestId;
    InstanceId responseInstanceId;
    StateRequestEntries entries;
};

} // namespace consolidator::core
