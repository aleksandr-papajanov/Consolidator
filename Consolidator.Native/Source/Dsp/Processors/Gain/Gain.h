#pragma once

#include <cstddef>
#include <cstdint>

#include "Dsp/Processors/DspDevice.h"

namespace consolidator::dsp
{

struct GainRuntimeState
{
    float gainDb = 0.0f;
    double linearGain = 1.0;
    bool isNeutral = true;
};

// Applies a configurable linear gain to every sample in the block.
class Gain final : public DspDevice
{
public:
    explicit Gain(DeviceId deviceId) noexcept
        : DspDevice(deviceId, detail::ElementKind::Device, 0)
    {
    }

    // Processes the block without allocating or changing authoritative state.
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
        const ParameterVariant& value) override;

private:
    bool ApplyOwnParameter(
        const core::StatePath& path,
        const ParameterVariant& value) override;

    void RecalculateRuntime() override;
    GainRuntimeState runtimeState_;
};

} // namespace consolidator::dsp
