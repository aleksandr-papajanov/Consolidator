#pragma once

#include <array>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <optional>

#include "Core/Domain/Ids/DspIds.h"
#include "Core/Domain/Ids/InstanceId.h"

namespace consolidator::core
{

enum class StateField : std::uint8_t
{
    InstanceId,
    Label,
    SelectedBank,
    BankId,
    GroupId,
    DspParameter,
    DspMarker,
    Mute,
    Solo
};

enum class StateMarkerId : std::uint8_t
{
    Bypass,
    Solo,
    Listen
};

// Fixed-size address for instance topology, DSP parameters and DSP markers.
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

    [[nodiscard]] static constexpr StatePath Label(InstanceId instance) noexcept
    {
        auto path = Instance(instance);
        path.field = StateField::Label;
        return path;
    }

    [[nodiscard]] static constexpr StatePath Device(dsp::DeviceId device) noexcept
    {
        StatePath path;
        path.deviceId = device;
        return path;
    }

    [[nodiscard]] static constexpr StatePath InstanceMute(InstanceId instance) noexcept
    {
        auto path = Instance(instance);
        path.field = StateField::Mute;
        return path;
    }

    [[nodiscard]] static constexpr StatePath InstanceSolo(InstanceId instance) noexcept
    {
        auto path = Instance(instance);
        path.field = StateField::Solo;
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

    [[nodiscard]] std::optional<dsp::BankId> TryGetBankId() const noexcept
    {
        if (depth == 0)
        {
            return std::nullopt;
        }

        const auto node = static_cast<std::uint8_t>(nodes[0]);
        const auto first = static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0);
        const auto last = static_cast<std::uint8_t>(dsp::RouteNodeId::Bank6);
        if (node < first || node > last)
        {
            return std::nullopt;
        }

        return static_cast<dsp::BankId>(node - first);
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

    template <typename... NodeIds>
    [[nodiscard]] static constexpr StatePath DspMarker(
        dsp::DeviceId device,
        StateMarkerId marker,
        NodeIds... nodeIds) noexcept
    {
        static_assert(sizeof...(NodeIds) <= 3);
        StatePath path;
        path.deviceId = device;
        path.markerId = marker;
        path.nodes = {static_cast<dsp::RouteNodeId>(nodeIds)...};
        path.depth = sizeof...(NodeIds);
        path.field = StateField::DspMarker;
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

    [[nodiscard]] constexpr StateMarkerId GetMarkerId() const noexcept
    {
        assert(markerId.has_value());
        return *markerId;
    }

    [[nodiscard]] constexpr std::size_t GetDepth() const noexcept
    {
        return depth;
    }

    // Validates the route shape understood by the realtime reset dispatcher.
    // This is deliberately independent of authoritative state serialization.
    [[nodiscard]] constexpr bool IsValidResetTarget() const noexcept
    {
        if (!deviceId || depth > 2)
        {
            return false;
        }

        const auto isBank = [](dsp::RouteNodeId node) constexpr
        {
            const auto value = static_cast<std::uint8_t>(node);
            return value >= static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) &&
                   value <= static_cast<std::uint8_t>(dsp::RouteNodeId::Bank6);
        };
        const auto isFilter = [](dsp::RouteNodeId node) constexpr
        {
            const auto value = static_cast<std::uint8_t>(node);
            return value >= static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) &&
                   value <= static_cast<std::uint8_t>(dsp::RouteNodeId::Filter7);
        };
        const auto isDetectorFilter = [](dsp::RouteNodeId node) constexpr
        {
            return node == dsp::RouteNodeId::Filter1 ||
                   node == dsp::RouteNodeId::Filter2;
        };

        switch (*deviceId)
        {
        case dsp::DeviceId::MainInputGain:
        case dsp::DeviceId::MainOutputGain:
            return depth == 0;
        case dsp::DeviceId::Saturator:
        case dsp::DeviceId::Compressor:
            return depth == 0 ||
                   (depth == 1 && nodes[0] == dsp::RouteNodeId::Detector) ||
                   (depth == 2 &&
                    nodes[0] == dsp::RouteNodeId::Detector &&
                    isDetectorFilter(nodes[1]));
        case dsp::DeviceId::Equalizer:
            return depth == 0 ||
                   (depth == 1 && isBank(nodes[0])) ||
                   (depth == 2 && isBank(nodes[0]) && isFilter(nodes[1]));
        }
        return false;
    }

    [[nodiscard]] constexpr dsp::RouteNodeId GetNode(std::size_t index) const noexcept
    {
        return nodes[index];
    }

    [[nodiscard]] constexpr StatePath WithParameter(
        dsp::ParameterId parameter) const noexcept
    {
        auto result = *this;
        result.field = StateField::DspParameter;
        result.markerId.reset();
        result.parameterId = parameter;
        return result;
    }

    [[nodiscard]] constexpr StatePath WithMarker(
        StateMarkerId marker) const noexcept
    {
        auto result = *this;
        result.field = StateField::DspMarker;
        result.parameterId.reset();
        result.markerId = marker;
        return result;
    }

    [[nodiscard]] constexpr StatePath WithInstance(
        InstanceId instance) const noexcept
    {
        auto result = *this;
        result.instanceId = instance;
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
    std::optional<StateMarkerId> markerId;
    std::array<dsp::RouteNodeId, 3> nodes{};
    std::size_t depth = 0;

    friend constexpr bool operator==(const StatePath&, const StatePath&) noexcept = default;

    // Checks whether this path is a prefix match for a candidate address.
    [[nodiscard]] bool Matches(const StatePath& candidate) const noexcept
    {
        if (instanceId && instanceId != candidate.instanceId) return false;
        if (field && field != candidate.field) return false;
        if (deviceId && deviceId != candidate.deviceId) return false;
        if (parameterId && parameterId != candidate.parameterId) return false;
        if (markerId && markerId != candidate.markerId) return false;
        if (depth > candidate.depth) return false;
        for (std::size_t index = 0; index < depth; ++index)
        {
            if (nodes[index] != candidate.nodes[index]) return false;
        }
        return true;
    }
};

} // namespace consolidator::core
