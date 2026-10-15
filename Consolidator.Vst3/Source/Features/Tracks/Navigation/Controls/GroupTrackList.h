#pragma once

#include <cstdint>
#include <functional>
#include <optional>
#include <vector>

#include <JuceHeader.h>

#include "Features/Tracks/ViewModels/TracksViewModel.h"
#include "Features/Tracks/Bindings/TrackIntent.h"

namespace consolidator::vst3::features::tracks::navigation::controls
{

// Membership projection; navigation uses the existing shared target intent.
class GroupTrackList final : public juce::Component, private juce::ListBoxModel
{
public:
    GroupTrackList(const tracks::viewmodels::TracksViewModel& tracks,
        std::function<void(const tracks::bindings::TrackIntent&)> emit);
    void Refresh(std::optional<std::uint32_t> group);
    void RefreshLevels();
    void resized() override;
    void enablementChanged() override;

private:
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& graphics, int width, int height, bool selected) override;
    juce::String getTooltipForRow(int row) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& event) override;
    const tracks::viewmodels::TracksViewModel& tracks_;
    std::function<void(const tracks::bindings::TrackIntent&)> emit_;
    std::optional<std::uint32_t> group_;
    std::vector<tracks::viewmodels::GroupTrack> rows_;
    juce::ListBox list_{ "Group tracks", this };
};

} // namespace consolidator::vst3::features::tracks::navigation::controls
