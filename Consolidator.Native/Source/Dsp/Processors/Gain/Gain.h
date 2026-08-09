#pragma once

#include <cstddef>
#include <cstdint>

#include "Dsp/Processors/DspDevice.h"

namespace consolidator::dsp
{

struct GainRuntimeState
{
    float gainDb = 0.0f;
    bool bypass = false;
    double linearGain = 1.0;
    bool isNeutral = true;
};

class Gain final : public DspDevice
{
public:
    explicit Gain(DeviceId deviceId) noexcept
        : DspDevice(deviceId, detail::ElementKind::Device, 0)
    {
    }

    void Process(
        const double* input,
        double* output,
        std::size_t frameCount,
        std::size_t channelCount) override;


    [[nodiscard]] const GainRuntimeState& GetRuntimeState() const noexcept
    {
        return runtimeState_;
    }

    [[nodiscard]] bool IsNeutral() const noexcept override
    {
        return runtimeState_.isNeutral;
    }

    bool StageRuntimeUpdate(
        const core::StatePath& path,
        const ParameterValue& value) override;

private:
    void RecalculateRuntime() override;
    GainRuntimeState runtimeState_;
};

} // namespace consolidator::dsp
