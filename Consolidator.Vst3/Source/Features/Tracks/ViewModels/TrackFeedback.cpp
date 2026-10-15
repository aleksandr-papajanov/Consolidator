#include "Features/Tracks/ViewModels/TrackFeedback.h"

#include <utility>

namespace consolidator::vst3::features::tracks::viewmodels
{

bool TrackFeedback::Bypass(std::uint64_t instance, const std::string& item, bool committed) const
{
    const auto found = bypass_.find({ instance, item });
    return found == bypass_.end() ? committed : found->second;
}

void TrackFeedback::SetBypass(std::uint64_t instance, std::string item, bool value)
{
    bypass_[{ instance, std::move(item) }] = value;
}

void TrackFeedback::ConfirmBypass(std::uint64_t instance, const std::string& item, bool committed)
{
    const auto found = bypass_.find({ instance, item });
    if (found != bypass_.end() && found->second == committed)
    {
        bypass_.erase(found);
    }
}

void TrackFeedback::Clear()
{
    bypass_.clear();
}

} // namespace consolidator::vst3::features::tracks::viewmodels
