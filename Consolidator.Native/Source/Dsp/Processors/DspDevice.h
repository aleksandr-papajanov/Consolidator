#pragma once

#include <cstddef>
#include <cstdint>

#include "Core/Ids/DspIds.h"
#include "Core/Parameters/ParameterValue.h"
#include "Core/State/StateProtocol.h"

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

    virtual bool StageRuntimeUpdate(
        const core::StatePath& path,
        const ParameterValue& value)
    {
        (void)path;
        (void)value;
        return false;
    }

    virtual void CommitRuntimeUpdates()
    {
        RecalculateRuntime();
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

protected:
    virtual bool ApplyParameter(
        const core::StatePath& route,
        const ParameterValue& value,
        std::size_t depth)
    {
        if (route.GetDeviceId() != deviceId_ || depth != route.GetDepth())
        {
            return false;
        }

        if (!ApplyOwnParameter(route, value))
        {
            return false;
        }

        return true;
    }

    virtual bool ApplyOwnParameter(
        const core::StatePath& route,
        const ParameterValue& value)
    {
        (void)route;
        (void)value;
        return false;
    }

    virtual void RecalculateRuntime() = 0;

private:
    DeviceId deviceId_;
    detail::ElementKind elementKind_;
    std::uint8_t elementIndex_;
};

} // namespace consolidator::dsp
