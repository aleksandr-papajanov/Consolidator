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
