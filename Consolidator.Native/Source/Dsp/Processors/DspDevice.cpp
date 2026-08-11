#include "Dsp/Processors/DspDevice.h"

namespace consolidator::dsp
{

DspDevice::DspDevice(
    DeviceId deviceId,
    detail::ElementKind elementKind,
    std::uint8_t elementIndex) noexcept
    : deviceId_(deviceId)
    , elementKind_(elementKind)
    , elementIndex_(elementIndex)
{
}

DspDevice::~DspDevice() = default;

void DspDevice::Prepare(
    double sampleRate,
    std::size_t channelCount)
{
    (void)sampleRate;
    (void)channelCount;
}

bool DspDevice::StageRuntimeUpdate(
    const core::StatePath& path,
    const ParameterVariant& value)
{
    (void)path;
    (void)value;
    return false;
}

void DspDevice::CommitRuntimeUpdates()
{
    RecalculateRuntime();
}

bool DspDevice::ApplyProcessingState(
    const core::StatePath& target,
    bool active)
{
    return ApplyProcessingStateAtDepth(target, active, 0);
}

bool DspDevice::ApplyProcessingStateAtDepth(
    const core::StatePath& target,
    bool active,
    std::size_t depth)
{
    if (target.GetDeviceId() != deviceId_ || depth != target.GetDepth())
    {
        return false;
    }
    active_ = active;
    return true;
}

bool DspDevice::ApplyMonitoringState(
    const core::StatePath& target,
    bool enabled)
{
    return ApplyMonitoringState(target, enabled, 0);
}

bool DspDevice::ApplyMonitoringState(
    const core::StatePath& target,
    bool enabled,
    std::size_t depth)
{
    (void)target;
    (void)enabled;
    (void)depth;
    return false;
}

void DspDevice::Reset() noexcept
{
}

bool DspDevice::Reset(
    const core::StatePath& path,
    std::size_t depth) noexcept
{
    if (depth == 0 &&
        (!path.deviceId || *path.deviceId != deviceId_))
    {
        return false;
    }

    if (depth != path.depth)
    {
        return false;
    }

    Reset();
    return true;
}

bool DspDevice::ApplyParameter(
    const core::StatePath& route,
    const ParameterVariant& value,
    std::size_t depth)
{
    if (route.GetDeviceId() != deviceId_ || depth != route.GetDepth())
    {
        return false;
    }

    return ApplyOwnParameter(route, value);
}

bool DspDevice::ApplyOwnParameter(
    const core::StatePath& route,
    const ParameterVariant& value)
{
    (void)route;
    (void)value;
    return false;
}

} // namespace consolidator::dsp
