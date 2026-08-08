#pragma once

#include <cstddef>
#include <cstdint>

#include "Core/Settings/DspDeviceSettings.h"
#include "Dsp/Parameters/DspParameter.h"
#include "Dsp/Processors/DspDevice.h"

namespace consolidator::dsp
{

struct GainState
{
    DspParameter<float> gainDb{
        ParameterId::Gain,
        static_cast<float>(core::settings::GainDefaults::kDefaultGainDb),
        static_cast<float>(core::settings::GainDefaults::kMinGainDb),
        static_cast<float>(core::settings::GainDefaults::kMaxGainDb)};
    DspParameter<bool> bypass{ParameterId::Bypass, false};
};

struct GainRuntimeState
{
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


    [[nodiscard]] const GainState& GetState() const noexcept
    {
        return state_;
    }

    [[nodiscard]] const GainRuntimeState& GetRuntimeState() const noexcept
    {
        return runtimeState_;
    }

    [[nodiscard]] bool IsNeutral() const noexcept override
    {
        return runtimeState_.isNeutral;
    }

private:
    void RecalculateRuntime() override;
    GainState state_;
    GainRuntimeState runtimeState_;
};

} // namespace consolidator::dsp
