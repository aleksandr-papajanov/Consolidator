#include "Features/Tracks/Navigation/Controls/TrackNavigationControl.h"

#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::tracks::navigation::controls
{

namespace
{
constexpr int kBankWidth = shared::ui::kBankCellSize + 1;
constexpr int kTrackWidth = 136;
}

TrackNavigationControl::TrackNavigationControl(viewmodels::TracksViewModel& tracks,
    std::function<void(const bindings::TrackIntent&)> emit)
    : tracks_(tracks), banks_(tracks, emit), groupTracks_(tracks, emit),
      groups_(tracks, [this](std::uint32_t group) { BrowseGroup(group); })
{
    // The group row extends above the graph; only children intercept input.
    setInterceptsMouseClicks(false, true);
    addAndMakeVisible(banks_);
    addAndMakeVisible(groupTracks_);
    addAndMakeVisible(groups_);
}

void TrackNavigationControl::Refresh()
{
    if (groupBrowse_ && (groupBrowse_->origin.instance != tracks_.FocusedInstance()
        || groupBrowse_->origin.bank != tracks_.FocusedBank()
        || tracks_.GroupTracks(groupBrowse_->group).empty()))
    {
        groupBrowse_.reset();
    }
    const auto group = groupBrowse_ ? std::optional{ groupBrowse_->group } : tracks_.FocusedGroup();
    groups_.Refresh(group);
    groupTracks_.Refresh(group);
    banks_.repaint();
}

void TrackNavigationControl::BrowseGroup(std::uint32_t group)
{
    groupBrowse_ = GroupBrowse{ group, { tracks_.FocusedInstance(), tracks_.FocusedBank() } };
    Refresh();
}

void TrackNavigationControl::RefreshLevels() { groupTracks_.RefreshLevels(); }
int TrackNavigationControl::ContentWidth() const { return kBankWidth + kTrackWidth; }

void TrackNavigationControl::resized()
{
    const int rowHeight = shared::ui::kBankCellSize;
    groups_.setBounds(kBankWidth, 0, groups_.PreferredWidth(), rowHeight);
    banks_.setBounds(0, rowHeight, kBankWidth, getHeight() - rowHeight);
    groupTracks_.setBounds(kBankWidth, rowHeight, kTrackWidth, getHeight() - rowHeight);
}

} // namespace consolidator::vst3::features::tracks::navigation::controls
