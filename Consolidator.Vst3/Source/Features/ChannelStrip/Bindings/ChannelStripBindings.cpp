#include "Features/ChannelStrip/Bindings/ChannelStripBindings.h"

#include <utility>

#include "Features/ChannelStrip/Presentation/ProcessorCatalog.h"
#include "Features/ChannelStrip/Compression/Bindings/CompressionBindings.h"

namespace consolidator::vst3::features::channelstrip::bindings
{

ChannelStripBindings::ChannelStripBindings(const shared::clients::TargetStateClient& state,
    shared::editing::DialGestureSession& gesture, shared::controls::parameter::bindings::ParameterBindingActions actions)
    : gainStaging_(state, gesture), parameters_(state, gesture, std::move(actions))
{
}

void ChannelStripBindings::ApplySnapshot()
{
    gainStaging_.ApplySnapshot();
    parameters_.ApplySnapshot(presentation::kParameterSections, gainStaging_.RuntimeOverlay());
    compression::bindings::RefreshAvailability(parameters_);
}

ChannelStripUpdate ChannelStripBindings::ApplyChanged(const shared::clients::ParameterEntry& entry)
{
    const bool runtimeOverlayChanged = gainStaging_.ApplyChanged(entry);
    const bool parameterChanged = parameters_.ApplyChanged(entry);
    const bool availabilityChanged = entry.path == "compression.type" && parameterChanged;
    if (availabilityChanged)
    {
        compression::bindings::RefreshAvailability(parameters_);
    }
    return { parameterChanged, runtimeOverlayChanged, availabilityChanged };
}

gainstaging::bindings::GainStagingBindings& ChannelStripBindings::GainStaging() { return gainStaging_; }
shared::controls::parameter::bindings::ParameterBindings& ChannelStripBindings::Parameters() { return parameters_; }

} // namespace consolidator::vst3::features::channelstrip::bindings
