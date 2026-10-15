#pragma once

#include "Features/ChannelStrip/Input/Bindings/InputBindings.h"
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
    [[nodiscard]] input::bindings::InputBindings& Input();
    [[nodiscard]] shared::controls::parameter::bindings::ParameterBindings& Parameters();

private:
    input::bindings::InputBindings input_;
    shared::controls::parameter::bindings::ParameterBindings parameters_;
};

} // namespace consolidator::vst3::features::channelstrip::bindings
