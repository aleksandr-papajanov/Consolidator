#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <type_traits>
#include <utility>

#include "Core/Parameters/DspParameter.h"

namespace consolidator::dsp
{

class DspDevice
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
        core::StateResponseEntries& output) const
    {
        (void)query;
        (void)output;
    }

    core::StateWriteStatus WriteState(
        const core::StateEntry& entry,
        core::StateResponseEntries& applied)
    {
        if (entry.path.field != core::StateField::DspParameter ||
            !entry.path.deviceId || !entry.path.parameterId)
        {
            return core::StateWriteStatus::NotHandled;
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

        if (!parameterValue)
        {
            core::StateEntry rejected{entry.path, entry.value};
            rejected.status = core::StateWriteStatus::Rejected;
            (void)applied.TryAppend(std::move(rejected));
            return core::StateWriteStatus::Rejected;
        }
        if (!WriteParameter(route, *parameterValue, 0))
        {
            const auto previousSize = applied.size;
            ReadState(entry.path, applied);
            const auto status = applied.size != previousSize
                ? core::StateWriteStatus::Unchanged
                : core::StateWriteStatus::Rejected;
            if (applied.size == previousSize)
            {
                core::StateEntry rejected{entry.path, entry.value};
                rejected.status = status;
                (void)applied.TryAppend(std::move(rejected));
            }
            else
            {
                for (std::size_t index = previousSize; index < applied.size; ++index)
                {
                    applied.entries[index].status = status;
                }
            }
            return status;
        }

        const auto previousSize = applied.size;
        ReadState(entry.path, applied);
        for (std::size_t index = previousSize; index < applied.size; ++index)
        {
            applied.entries[index].status = core::StateWriteStatus::Applied;
        }
        return core::StateWriteStatus::Applied;
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
            core::StateEntry entry{candidate, core::StateValue{parameter.value}};
            if constexpr (!std::is_same_v<T, bool>)
            {
                entry.physicalMinimum = ParameterValue{parameter.minimum};
                entry.physicalMaximum = ParameterValue{parameter.maximum};
                entry.minimum = entry.physicalMinimum;
                entry.maximum = entry.physicalMaximum;
            }
            (void)snapshot.TryAppend(std::move(entry));
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
