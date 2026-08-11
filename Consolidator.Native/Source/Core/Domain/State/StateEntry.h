#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <utility>
#include <variant>
#include <vector>

#include "Core/Domain/Ids/DspIds.h"
#include "Core/Domain/Ids/GroupId.h"
#include "Core/Domain/Ids/InstanceId.h"
#include "Core/Domain/ParameterVariant.h"
#include "Core/Domain/State/StatePath.h"

namespace consolidator::core
{

// Values supported by topology and DSP state protocol entries.
using StateValue = std::variant<
    std::monostate,
    bool,
    std::int32_t,
    float,
    InstanceId,
    dsp::BankId,
    GroupId>;

enum class StateWriteStatus : std::uint8_t
{
    NotHandled,
    Applied,
    Unchanged,
    Rejected
};

// Addressed state value plus optional ranges and write result metadata.
struct StateEntry
{
    StatePath path;
    StateValue value;
    std::optional<dsp::ParameterVariant> physicalMinimum;
    std::optional<dsp::ParameterVariant> physicalMaximum;
    std::optional<dsp::ParameterVariant> minimum;
    std::optional<dsp::ParameterVariant> maximum;
    std::optional<StateWriteStatus> status;
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

// Bounded allocation-free list used by state requests and responses.
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
using StateResponseEntries = FixedStateList<512>;
using StateSnapshot = StateResponseEntries;

} // namespace consolidator::core
