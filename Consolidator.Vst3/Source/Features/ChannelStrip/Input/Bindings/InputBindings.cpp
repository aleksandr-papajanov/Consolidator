#include "Features/ChannelStrip/Input/Bindings/InputBindings.h"

#include <string>
#include <string_view>

#include "Shared/Protocol/ProtocolFrame.h"

namespace consolidator::vst3::features::channelstrip::input::bindings
{

namespace
{
constexpr std::string_view kLevelPath = "input_gain.level";
constexpr std::string_view kLevelerPath = "input_gain.leveler";
}

InputBindings::InputBindings(const shared::clients::TargetStateClient& state,
    shared::editing::DialGestureSession& gesture)
    : state_(state), gesture_(gesture)
{
}

void InputBindings::ApplySnapshot()
{
    leveler_.SetEnabled(shared::protocol::Boolean(state_.Require(std::string(kLevelerPath)).value));
}

bool InputBindings::ApplyChanged(const shared::clients::ParameterEntry& entry)
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

shared::controls::dial::DialRuntimeOverlayBinding InputBindings::RuntimeOverlay() const
{
    return { kLevelPath, &leveler_ };
}

std::string_view InputBindings::OverlayPath() const
{
    return kLevelPath;
}

bool InputBindings::IsAutomatedValue(std::string_view path) const
{
    return path == kLevelPath && leveler_.Enabled();
}

void InputBindings::ApplyTelemetry(double slow, double fast)
{
    leveler_.ApplyTelemetry(slow, fast);
}

void InputBindings::ClearTelemetry()
{
    leveler_.ClearTelemetry();
}

} // namespace consolidator::vst3::features::channelstrip::input::bindings
