#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>

#include "Core/Parameters/DspParameter.h"
#include "Core/State/IStateNode.h"

namespace consolidator::dsp
{

class DspDevice : public core::IStateNode
{
public:
    DspDevice(
        DeviceId deviceId,
        detail::ElementKind elementKind,
        std::uint8_t elementIndex) noexcept
        : deviceId_(deviceId)
        , elementKind_(elementKind)
        , elementIndex_(elementIndex)
    {
    }

    virtual ~DspDevice() = default;

    virtual void Process(
        const double* input,
        double* output,
        std::size_t frameCount,
        std::size_t channelCount) = 0;

    virtual bool WriteParameter(
        const core::StatePath& route,
        const ParameterValue& value,
        std::size_t depth)
    {
        if (route.GetDeviceId() != deviceId_ || depth != route.GetDepth())
        {
            return false;
        }

        if (!WriteOwnParameter(route, value))
        {
            return false;
        }

        RecalculateRuntime();
        return true;
    }

    [[nodiscard]] DeviceId GetDeviceId() const noexcept
    {
        return deviceId_;
    }

    [[nodiscard]] detail::ElementKind GetElementKind() const noexcept
    {
        return elementKind_;
    }

    [[nodiscard]] std::uint8_t GetElementIndex() const noexcept
    {
        return elementIndex_;
    }

    [[nodiscard]] virtual bool IsNeutral() const noexcept = 0;

    virtual void ReadState(
        const core::StatePath& query,
        core::StateResponseEntries& output) const override = 0;

    bool WriteState(
        const core::StateEntry& entry,
        core::StateResponseEntries& applied) override
    {
        if (entry.path.field != core::StateField::DspParameter ||
            !entry.path.deviceId || !entry.path.parameterId)
        {
            return false;
        }

        core::StatePath route{*entry.path.deviceId, *entry.path.parameterId};
        for (std::size_t index = 0; index < entry.path.depth; ++index)
        {
            route = route.WithNode(entry.path.nodes[index]);
        }

        const auto parameterValue = std::visit(
            [](const auto& value) -> std::optional<ParameterValue>
            {
                using ValueType = std::decay_t<decltype(value)>;
                if constexpr (std::is_same_v<ValueType, bool> ||
                              std::is_same_v<ValueType, std::int32_t> ||
                              std::is_same_v<ValueType, float>)
                {
                    return ParameterValue{value};
                }

                return std::nullopt;
            },
            entry.value);

        if (!parameterValue || !WriteParameter(route, *parameterValue, 0))
        {
            return false;
        }

        ReadState(entry.path, applied);
        return true;
    }

protected:
    template <typename T>
    static void AppendParameter(
        const core::StatePath& path,
        core::StateResponseEntries& snapshot,
        core::StatePath route,
        const DspParameter<T>& parameter)
    {
        auto candidate = core::ToStatePath(route.WithParameter(parameter.id));
        candidate.instanceId = path.instanceId;
        if (path.Matches(candidate))
        {
            (void)snapshot.TryAppend(core::StateEntry{candidate, core::StateValue{parameter.value}});
        }
    }

    virtual bool WriteOwnParameter(
        const core::StatePath& route,
        const ParameterValue& value)
    {
        return false;
    }

    virtual void RecalculateRuntime() = 0;

private:
    DeviceId deviceId_;
    detail::ElementKind elementKind_;
    std::uint8_t elementIndex_;
};

} // namespace consolidator::dsp
