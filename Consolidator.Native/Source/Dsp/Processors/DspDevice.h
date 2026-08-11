#pragma once

#include <cstddef>
#include <cstdint>

#include "Core/Domain/Ids/DspIds.h"
#include "Core/Domain/ParameterVariant.h"
#include "Core/Domain/State/StatePath.h"

namespace consolidator::dsp
{

// Base contract for a runtime-updatable, real-time DSP processor.
class DspDevice
{
public:
    DspDevice(
        DeviceId deviceId,
        detail::ElementKind elementKind,
        std::uint8_t elementIndex) noexcept;

    virtual ~DspDevice();

    // Prepares sample-rate-dependent state before the audio callback starts.
    virtual void Prepare(
        double sampleRate,
        std::size_t channelCount);

    virtual void Process(
        const double* inputLeft,
        const double* inputRight,
        double* outputLeft,
        double* outputRight,
        std::size_t frameCount) = 0;

    // Stages a value without recalculating dependent runtime data immediately.
    virtual bool StageRuntimeUpdate(
        const core::StatePath& path,
        const ParameterVariant& value);

    // Commits staged values and rebuilds derived runtime state once per batch.
    virtual void CommitRuntimeUpdates();

    bool ApplyProcessingState(
        const core::StatePath& target,
        bool active);

    bool ApplyMonitoringState(
        const core::StatePath& target,
        bool enabled);

    virtual void Reset() noexcept;

    // Resets the component addressed by a route segment during recursive dispatch.
    virtual bool Reset(
        const core::StatePath& path,
        std::size_t depth) noexcept;

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

    [[nodiscard]] bool IsActive() const noexcept
    {
        return active_;
    }

protected:
    // Recursive route dispatch used by composite devices.
    virtual bool ApplyProcessingStateAtDepth(
        const core::StatePath& target,
        bool active,
        std::size_t depth);

    virtual bool ApplyParameter(
        const core::StatePath& route,
        const ParameterVariant& value,
        std::size_t depth);

    virtual bool ApplyOwnParameter(
        const core::StatePath& route,
        const ParameterVariant& value);

    virtual void RecalculateRuntime() = 0;

    virtual bool ApplyMonitoringState(
        const core::StatePath& target,
        bool enabled,
        std::size_t depth);

private:
    DeviceId deviceId_;
    detail::ElementKind elementKind_;
    std::uint8_t elementIndex_;
    bool active_ = true;
};

} // namespace consolidator::dsp
