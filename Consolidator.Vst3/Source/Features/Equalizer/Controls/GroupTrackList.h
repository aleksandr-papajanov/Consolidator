#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <vector>

#include <JuceHeader.h>

#include "Features/Tracks/ViewModels/TracksViewModel.h"
#include "Features/Tracks/Bindings/TrackIntent.h"

namespace consolidator::vst3::features::equalizer::controls
{

// Membership projection; navigation uses the existing shared target intent.
class GroupTrackList final : public juce::Component, private juce::ListBoxModel
{
public:
    GroupTrackList(const tracks::viewmodels::TracksViewModel& tracks,
        std::function<void(const tracks::bindings::TrackIntent&)> emit);
    void Refresh();
    void resized() override;
    void enablementChanged() override;

private:
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& graphics, int width, int height, bool selected) override;
    juce::String getTooltipForRow(int row) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& event) override;
    struct TrackRow
    {
        std::uint64_t instance;
        int bank;
        std::string name;
        bool operator==(const TrackRow&) const = default;
    };
    const tracks::viewmodels::TracksViewModel& tracks_;
    std::function<void(const tracks::bindings::TrackIntent&)> emit_;
    std::vector<TrackRow> rows_;
    juce::ListBox list_{ "Group tracks", this };
};

} // namespace consolidator::vst3::features::equalizer::controls
