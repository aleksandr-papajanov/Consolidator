#pragma once

#include <cstddef>
#include <cstdint>
#include <initializer_list>
#include <stdexcept>
#include <utility>

#include "Core/Domain/State/StateEntry.h"

namespace consolidator::test
{

inline core::StatePath DevicePath(
    core::InstanceId instanceId,
    dsp::DeviceId deviceId,
    dsp::ParameterId parameterId)
{
    return core::StatePath::DspParameter(
        instanceId,
        core::StatePath{deviceId, parameterId});
}

inline core::StatePath DevicePath(
    core::InstanceId instanceId,
    dsp::DeviceId deviceId,
    core::StateMarkerId markerId)
{
    return core::StatePath::DspMarker(
        deviceId,
        markerId).WithInstance(instanceId);
}

inline core::StatePath BankPath(
    core::InstanceId instanceId,
    dsp::BankId bankId,
    dsp::ParameterId parameterId)
{
    const auto bankNode = static_cast<dsp::RouteNodeId>(
        static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) +
        dsp::detail::ToIndex(bankId));
    return core::StatePath::DspParameter(
        instanceId,
        core::StatePath{dsp::DeviceId::Equalizer, parameterId, bankNode});
}

inline core::StatePath BankPath(
    core::InstanceId instanceId,
    dsp::BankId bankId,
    core::StateMarkerId markerId)
{
    const auto bankNode = static_cast<dsp::RouteNodeId>(
        static_cast<std::uint8_t>(dsp::RouteNodeId::Bank0) +
        dsp::detail::ToIndex(bankId));
    return core::StatePath::DspMarker(
        dsp::DeviceId::Equalizer,
        markerId,
        bankNode).WithInstance(instanceId);
}

inline core::StatePath FilterPath(
    core::InstanceId instanceId,
    dsp::BankId bankId,
    std::size_t filterIndex,
    dsp::ParameterId parameterId)
{
    const auto filterNode = static_cast<dsp::RouteNodeId>(
        static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
    return BankPath(instanceId, bankId, parameterId).WithNode(filterNode);
}

inline core::StatePath FilterPath(
    core::InstanceId instanceId,
    dsp::BankId bankId,
    std::size_t filterIndex,
    core::StateMarkerId markerId)
{
    const auto filterNode = static_cast<dsp::RouteNodeId>(
        static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
    return BankPath(instanceId, bankId, markerId).WithNode(filterNode);
}

inline core::StatePath DetectorPath(
    core::InstanceId instanceId,
    dsp::DeviceId deviceId,
    dsp::ParameterId parameterId)
{
    return core::StatePath::DspParameter(
        instanceId,
        core::StatePath{deviceId, parameterId, dsp::RouteNodeId::Detector});
}

inline core::StatePath DetectorPath(
    core::InstanceId instanceId,
    dsp::DeviceId deviceId,
    core::StateMarkerId markerId)
{
    return core::StatePath::DspMarker(
        deviceId,
        markerId,
        dsp::RouteNodeId::Detector).WithInstance(instanceId);
}

inline core::StatePath DetectorFilterPath(
    core::InstanceId instanceId,
    dsp::DeviceId deviceId,
    std::size_t filterIndex,
    dsp::ParameterId parameterId)
{
    const auto filterNode = static_cast<dsp::RouteNodeId>(
        static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
    return DetectorPath(instanceId, deviceId, parameterId).WithNode(filterNode);
}

inline core::StatePath DetectorFilterPath(
    core::InstanceId instanceId,
    dsp::DeviceId deviceId,
    std::size_t filterIndex,
    core::StateMarkerId markerId)
{
    const auto filterNode = static_cast<dsp::RouteNodeId>(
        static_cast<std::uint8_t>(dsp::RouteNodeId::Filter1) + filterIndex);
    return DetectorPath(instanceId, deviceId, markerId).WithNode(filterNode);
}

inline core::StateEntry Write(core::StatePath path, core::StateValue value)
{
    return {std::move(path), std::move(value)};
}

inline core::StateRequestEntries Entries(
    std::initializer_list<core::StateEntry> values)
{
    core::StateRequestEntries result;
    for (const auto& value : values)
    {
        if (!result.TryAppend(value))
        {
            throw std::runtime_error("test request exceeded fixed capacity");
        }
    }
    return result;
}

inline bool IsExactPath(
    const core::StatePath& first,
    const core::StatePath& second) noexcept
{
    return first.Matches(second) && second.Matches(first);
}

inline core::StatePath RuntimeTarget(core::StatePath parameterPath)
{
    parameterPath.field.reset();
    parameterPath.parameterId.reset();
    parameterPath.markerId.reset();
    return parameterPath;
}

} // namespace consolidator::test
