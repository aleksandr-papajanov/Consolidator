#include "Features/ChannelStrip/GainStaging/ViewModels/RuntimeDialViewModel.h"

#include <algorithm>

namespace consolidator::vst3::features::channelstrip::gainstaging::viewmodels
{

void RuntimeDialViewModel::SetEnabled(bool enabled)
{
    if (enabled_ != enabled)
    {
        ClearTelemetry();
    }
    enabled_ = enabled;
}

void RuntimeDialViewModel::ApplyTelemetry(std::optional<double> value, std::optional<double> fast)
{
    if (enabled_)
    {
        value_ = value;
        fast_ = fast;
    }
}

void RuntimeDialViewModel::ClearTelemetry()
{
    value_.reset();
    fast_.reset();
}

bool RuntimeDialViewModel::Enabled() const
{
    return enabled_;
}

std::optional<double> RuntimeDialViewModel::DisplayValue() const
{
    return enabled_ ? value_ : std::nullopt;
}

std::optional<double> RuntimeDialViewModel::FastPosition() const
{
    return enabled_ && fast_ ? std::optional<double>(std::clamp((*fast_ + 6.0) / 12.0, 0.0, 1.0))
        : std::nullopt;
}

} // namespace consolidator::vst3::features::channelstrip::gainstaging::viewmodels
