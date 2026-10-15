#pragma once

#include <array>
#include <string_view>

#include "Shared/Clients/ParameterEntry.h"
#include "Shared/Clients/TargetStateClient.h"
#include "Shared/Editing/DialGestureSession.h"
#include "Shared/Controls/Dial/DialRuntimeOverlayViewModel.h"
#include "Features/ChannelStrip/GainStaging/Clients/GainTelemetryClient.h"
#include "Features/ChannelStrip/GainStaging/ViewModels/RuntimeDialViewModel.h"

namespace consolidator::vst3::features::channelstrip::gainstaging::bindings
{

inline constexpr std::array<std::string_view, 4> kRuntimePaths{
    "gain_staging.input_level", "gain_staging.input_target",
    "gain_staging.output_level", "gain_staging.output_target" };

class GainStagingBindings final
{
public:
    GainStagingBindings(const shared::clients::TargetStateClient& state, shared::editing::DialGestureSession& gesture);
    void ApplySnapshot();
    [[nodiscard]] bool ApplyChanged(const shared::clients::ParameterEntry& entry);
    [[nodiscard]] std::array<shared::controls::dial::DialRuntimeOverlayBinding, 4> RuntimeOverlays() const;
    [[nodiscard]] bool IsAutomatedValue(std::string_view path) const;
    void ApplyTelemetry(const clients::GainTelemetry& telemetry);
    void ClearTelemetry();

private:
    const shared::clients::TargetStateClient& state_;
    shared::editing::DialGestureSession& gesture_;
    std::array<viewmodels::RuntimeDialViewModel, 4> overlays_;
};

} // namespace consolidator::vst3::features::channelstrip::gainstaging::bindings
