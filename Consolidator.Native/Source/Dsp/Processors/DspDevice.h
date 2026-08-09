#pragma once

#include <cstddef>
#include <cstdint>

#include "Dsp/Processors/DspNode.h"
#include "Core/Parameters/DspParameter.h"
#include "Core/State/IStateSource.h"

namespace consolidator::dsp
{

class DspDevice : public DspNode, public core::IStateSource
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

    virtual bool ApplyParameter(
        const ParameterRoute& route,
        const ParameterValue& value,
        std::size_t depth)
    {
        if (route.GetDeviceId() != deviceId_ || depth != route.GetDepth())
        {
            return false;
        }

        if (!ApplyStateParameter(route, value))
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

    virtual void AppendState(
        const core::StatePath& path,
        core::StateSnapshot& snapshot) const override = 0;

protected:
    template <typename T>
    static void AppendParameter(
        const core::StatePath& path,
        core::StateSnapshot& snapshot,
        ParameterRoute route,
        const DspParameter<T>& parameter)
    {
        auto candidate = core::ToStatePath(route.WithParameter(parameter.id));
        candidate.instanceId = path.instanceId;
        if (path.Matches(candidate))
        {
            (void)snapshot.TryAppend(core::StateEntry{candidate, core::StateValue{parameter.value}});
        }
    }

    virtual bool ApplyStateParameter(
        const ParameterRoute& route,
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
