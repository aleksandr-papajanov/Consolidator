#pragma once

#include <cstddef>
#include <cstdint>

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Processors/IDspDevice.h"

namespace consolidator::dsp
{

struct GainState
{
    float gainDb = static_cast<float>(core::settings::GainDefaults::kDefaultGainDb);
    bool bypass = false;
};

struct GainRuntime
{
    double linearGain = 1.0;
};

class Gain final : public IDspDevice
{
public:
    explicit Gain(DeviceId deviceId) noexcept
        : deviceId_(deviceId)
    {
    }

    void Process(
        const double* input,
        double* output,
        std::size_t frameCount,
        std::size_t channelCount) override;

    void ApplyParameterChange(const ParameterChange& change) override;

    [[nodiscard]] DeviceId GetDeviceId() const noexcept override
    {
        return deviceId_;
    }

    [[nodiscard]] detail::ElementKind GetElementKind() const noexcept override
    {
        return detail::ElementKind::Device;
    }

    [[nodiscard]] std::uint8_t GetElementIndex() const noexcept override
    {
        return 0;
    }

    [[nodiscard]] const GainState& GetState() const noexcept
    {
        return state_;
    }

    [[nodiscard]] bool IsNeutral() const noexcept override
    {
        return state_.bypass || runtime_.linearGain == 1.0;
    }

private:
    void RecalculateRuntime();

    DeviceId deviceId_;
    GainState state_;
    GainRuntime runtime_;
};

} // namespace consolidator::dsp