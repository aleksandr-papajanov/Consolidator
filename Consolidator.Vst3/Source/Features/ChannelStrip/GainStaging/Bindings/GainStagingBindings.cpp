#include "Features/ChannelStrip/GainStaging/Bindings/GainStagingBindings.h"

#include <cstddef>
#include <string>
#include <string_view>

#include "Shared/Protocol/ProtocolFrame.h"

namespace consolidator::vst3::features::channelstrip::gainstaging::bindings
{

GainStagingBindings::GainStagingBindings(const shared::clients::TargetStateClient& state,
    shared::editing::DialGestureSession& gesture)
    : state_(state), gesture_(gesture)
{
}

void GainStagingBindings::ApplySnapshot()
{
    const auto input = shared::protocol::Boolean(state_.Require("gain_staging.input_leveler").value);
    const auto output = shared::protocol::Boolean(state_.Require("gain_staging.output_leveler").value);
    overlays_[0].SetEnabled(input);
    overlays_[1].SetEnabled(!input);
    overlays_[2].SetEnabled(output);
    overlays_[3].SetEnabled(!output);
}

bool GainStagingBindings::ApplyChanged(const shared::clients::ParameterEntry& entry)
{
    if (entry.path != "gain_staging.input_leveler" && entry.path != "gain_staging.output_leveler")
    {
        return false;
    }
    ApplySnapshot();
    if (gesture_.Active() && IsAutomatedValue(gesture_.Path()))
    {
        gesture_.Cancel();
    }
    return true;
}

std::array<shared::controls::dial::DialRuntimeOverlayBinding, 4> GainStagingBindings::RuntimeOverlays() const
{
    std::array<shared::controls::dial::DialRuntimeOverlayBinding, 4> bindings{};
    for (std::size_t index = 0; index < bindings.size(); ++index)
    {
        bindings[index] = { kRuntimePaths[index], &overlays_[index] };
    }
    return bindings;
}

bool GainStagingBindings::IsAutomatedValue(std::string_view path) const
{
    for (std::size_t index = 0; index < kRuntimePaths.size(); ++index)
    {
        if (path == kRuntimePaths[index])
        {
            return overlays_[index].Enabled();
        }
    }
    return false;
}

void GainStagingBindings::ApplyTelemetry(const clients::GainTelemetry& telemetry)
{
    overlays_[0].ApplyTelemetry(telemetry.inputLevel, telemetry.inputFast);
    overlays_[1].ApplyTelemetry(telemetry.inputRms);
    overlays_[2].ApplyTelemetry(telemetry.outputLevel, telemetry.outputFast);
    overlays_[3].ApplyTelemetry(telemetry.outputRms);
}

void GainStagingBindings::ClearTelemetry()
{
    for (auto& overlay : overlays_)
    {
        overlay.ClearTelemetry();
    }
}

} // namespace consolidator::vst3::features::channelstrip::gainstaging::bindings
