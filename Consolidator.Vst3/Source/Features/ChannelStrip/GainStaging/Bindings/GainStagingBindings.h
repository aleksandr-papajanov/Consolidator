#pragma once

#include <string_view>

#include "Shared/Clients/ParameterEntry.h"
#include "Shared/Clients/TargetStateClient.h"
#include "Shared/Editing/DialGestureSession.h"
#include "Shared/Controls/Dial/DialRuntimeOverlayViewModel.h"
#include "Features/ChannelStrip/GainStaging/ViewModels/InputLevelerViewModel.h"

namespace consolidator::vst3::features::channelstrip::gainstaging::bindings
{

class GainStagingBindings final
{
public:
    GainStagingBindings(const shared::clients::TargetStateClient& state, shared::editing::DialGestureSession& gesture);
    void ApplySnapshot();
    [[nodiscard]] bool ApplyChanged(const shared::clients::ParameterEntry& entry);
    [[nodiscard]] shared::controls::dial::DialRuntimeOverlayBinding RuntimeOverlay() const;
    [[nodiscard]] std::string_view OverlayPath() const;
    [[nodiscard]] bool IsAutomatedValue(std::string_view path) const;
    void ApplyTelemetry(double slow, double fast);
    void ClearTelemetry();

private:
    const shared::clients::TargetStateClient& state_;
    shared::editing::DialGestureSession& gesture_;
    features::channelstrip::gainstaging::viewmodels::InputLevelerViewModel leveler_;
};

} // namespace consolidator::vst3::features::channelstrip::gainstaging::bindings
