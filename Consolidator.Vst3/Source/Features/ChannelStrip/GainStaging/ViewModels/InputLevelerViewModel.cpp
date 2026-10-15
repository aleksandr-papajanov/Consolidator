#include "Features/ChannelStrip/GainStaging/ViewModels/InputLevelerViewModel.h"

#include <algorithm>

namespace consolidator::vst3::features::channelstrip::gainstaging::viewmodels
{

void InputLevelerViewModel::SetEnabled(bool enabled)
{
    enabled_ = enabled;
    if (!enabled)
    {
        ClearTelemetry();
    }
}

void InputLevelerViewModel::ApplyTelemetry(double slow, double fast)
{
    if (enabled_)
    {
        telemetry_ = std::pair{ slow, fast };
    }
}

void InputLevelerViewModel::ClearTelemetry() { telemetry_.reset(); }
bool InputLevelerViewModel::Enabled() const { return enabled_; }

std::optional<double> InputLevelerViewModel::DisplayValue() const
{
    if (!enabled_ || !telemetry_)
    {
        return {};
    }
    return telemetry_->first;
}

std::optional<double> InputLevelerViewModel::FastPosition() const
{
    if (!enabled_ || !telemetry_)
    {
        return {};
    }
    return std::clamp((telemetry_->second + 6) / 12, 0.0, 1.0);
}

} // namespace consolidator::vst3::features::channelstrip::gainstaging::viewmodels
