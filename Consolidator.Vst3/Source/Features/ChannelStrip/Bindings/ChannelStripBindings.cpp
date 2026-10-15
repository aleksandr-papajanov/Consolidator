#include "Features/ChannelStrip/Bindings/ChannelStripBindings.h"

#include <utility>

#include "Features/ChannelStrip/Presentation/ProcessorCatalog.h"

namespace consolidator::vst3::features::channelstrip::bindings
{

ChannelStripBindings::ChannelStripBindings(const shared::clients::TargetStateClient& state,
    shared::editing::DialGestureSession& gesture, shared::controls::parameter::bindings::ParameterBindingActions actions)
    : input_(state, gesture), parameters_(state, gesture, std::move(actions))
{
}

void ChannelStripBindings::ApplySnapshot()
{
    input_.ApplySnapshot();
    parameters_.ApplySnapshot(presentation::kSections, input_.RuntimeOverlay());
}

ChannelStripUpdate ChannelStripBindings::ApplyChanged(const shared::clients::ParameterEntry& entry)
{
    const bool runtimeOverlayChanged = input_.ApplyChanged(entry);
    const bool parameterChanged = parameters_.ApplyChanged(entry);
    return { parameterChanged, runtimeOverlayChanged };
}

input::bindings::InputBindings& ChannelStripBindings::Input() { return input_; }
shared::controls::parameter::bindings::ParameterBindings& ChannelStripBindings::Parameters() { return parameters_; }

} // namespace consolidator::vst3::features::channelstrip::bindings
