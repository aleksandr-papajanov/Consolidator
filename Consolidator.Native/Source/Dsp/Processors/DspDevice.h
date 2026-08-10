#pragma once

#include <cstddef>
#include <cstdint>

#include "Core/Domain/Ids/DspIds.h"
#include "Core/Domain/ParameterVariant.h"
#include "Core/Domain/State/StateProtocol.h"

namespace consolidator::dsp
{

class DspDevice
{
public:
    DspDevice(
        DeviceId deviceId,
        detail::ElementKind elementKind,
        std::uint8_t elementIndex) noexcept;

    virtual ~DspDevice();

    virtual void Process(
        const double* input,
        double* output,
        std::size_t frameCount,
        std::size_t channelCount) = 0;

    virtual bool StageRuntimeUpdate(
        const core::StatePath& path,
        const ParameterVariant& value);

    virtual void CommitRuntimeUpdates();

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
        const ParameterVariant& value,
        std::size_t depth);

    virtual bool ApplyOwnParameter(
        const core::StatePath& route,
        const ParameterVariant& value);

    virtual void RecalculateRuntime() = 0;

private:
    DeviceId deviceId_;
    detail::ElementKind elementKind_;
    std::uint8_t elementIndex_;
};

} // namespace consolidator::dsp
