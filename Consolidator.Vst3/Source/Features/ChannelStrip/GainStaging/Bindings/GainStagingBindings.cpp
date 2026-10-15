#include "Features/ChannelStrip/GainStaging/Bindings/GainStagingBindings.h"

#include <string>
#include <string_view>

#include "Shared/Protocol/ProtocolFrame.h"

namespace consolidator::vst3::features::channelstrip::gainstaging::bindings
{

namespace
{
constexpr std::string_view kLevelPath = "gain_staging.input_level";
constexpr std::string_view kLevelerPath = "gain_staging.input_leveler";
}

GainStagingBindings::GainStagingBindings(const shared::clients::TargetStateClient& state,
    shared::editing::DialGestureSession& gesture)
    : state_(state), gesture_(gesture)
{
}

void GainStagingBindings::ApplySnapshot()
{
    leveler_.SetEnabled(shared::protocol::Boolean(state_.Require(std::string(kLevelerPath)).value));
}

bool GainStagingBindings::ApplyChanged(const shared::clients::ParameterEntry& entry)
{
    if (std::string_view(entry.path) != kLevelerPath)
    {
        return false;
    }

    leveler_.SetEnabled(shared::protocol::Boolean(entry.value));
    if (leveler_.Enabled() && gesture_.Active() && std::string_view(gesture_.Path()) == kLevelPath)
    {
        gesture_.Cancel();
    }
    return true;
}

shared::controls::dial::DialRuntimeOverlayBinding GainStagingBindings::RuntimeOverlay() const
{
    return { kLevelPath, &leveler_ };
}

std::string_view GainStagingBindings::OverlayPath() const
{
    return kLevelPath;
}

bool GainStagingBindings::IsAutomatedValue(std::string_view path) const
{
    return path == kLevelPath && leveler_.Enabled();
}

void GainStagingBindings::ApplyTelemetry(double slow, double fast)
{
    leveler_.ApplyTelemetry(slow, fast);
}

void GainStagingBindings::ClearTelemetry()
{
    leveler_.ClearTelemetry();
}

} // namespace consolidator::vst3::features::channelstrip::gainstaging::bindings
