#pragma once

#include <cstdint>
#include <functional>
#include <optional>

#include <JuceHeader.h>

#include "Features/Tracks/Navigation/Controls/BankColumnControl.h"
#include "Features/Tracks/Navigation/Controls/GroupSelectorControl.h"
#include "Features/Tracks/Navigation/Controls/GroupTrackList.h"
#include "Shared/Clients/TargetStateClient.h"

namespace consolidator::vst3::features::tracks::navigation::controls
{

// Borrows the shared target; browsing a group never changes it.
class TrackNavigationControl final : public juce::Component
{
public:
    TrackNavigationControl(viewmodels::TracksViewModel& tracks,
        std::function<void(const bindings::TrackIntent&)> emit);
    void Refresh();
    void RefreshLevels();
    [[nodiscard]] int ContentWidth() const;
    void resized() override;

private:
    struct GroupBrowse
    {
        std::uint32_t group;
        shared::clients::UiTarget origin;
    };

    void BrowseGroup(std::uint32_t group);
    viewmodels::TracksViewModel& tracks_;
    BankColumnControl banks_;
    GroupTrackList groupTracks_;
    GroupSelectorControl groups_;
    std::optional<GroupBrowse> groupBrowse_;
};

} // namespace consolidator::vst3::features::tracks::navigation::controls
