#pragma once

#include "Features/ChannelStrip/GainStaging/Bindings/GainStagingBindings.h"
#include "Shared/Controls/Parameter/Bindings/ParameterBindings.h"

namespace consolidator::vst3::features::channelstrip::bindings
{

struct ChannelStripUpdate
{
    bool parameterChanged;
    bool runtimeOverlayChanged;
};

class ChannelStripBindings final
{
public:
    ChannelStripBindings(const shared::clients::TargetStateClient& state, shared::editing::DialGestureSession& gesture,
        shared::controls::parameter::bindings::ParameterBindingActions actions);
    void ApplySnapshot();
    [[nodiscard]] ChannelStripUpdate ApplyChanged(const shared::clients::ParameterEntry& entry);
    [[nodiscard]] gainstaging::bindings::GainStagingBindings& GainStaging();
    [[nodiscard]] shared::controls::parameter::bindings::ParameterBindings& Parameters();

private:
    gainstaging::bindings::GainStagingBindings gainStaging_;
    shared::controls::parameter::bindings::ParameterBindings parameters_;
};

} // namespace consolidator::vst3::features::channelstrip::bindings
