#include "Features/Equalizer/Controls/GroupTrackList.h"

#include <algorithm>
#include <utility>

#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::equalizer::controls
{

GroupTrackList::GroupTrackList(const tracks::viewmodels::TracksViewModel& tracks,
    std::function<void(const tracks::bindings::TrackIntent&)> emit)
    : tracks_(tracks), emit_(std::move(emit))
{
    list_.setRowHeight(shared::ui::kBankCellSize);
    list_.setColour(juce::ListBox::backgroundColourId, shared::ui::kAnalyzerBackground);
    list_.setOutlineThickness(0);
    list_.setMultipleSelectionEnabled(false);
    list_.getViewport()->setScrollBarThickness(8);
    addAndMakeVisible(list_);
}

void GroupTrackList::Refresh()
{
    std::vector<TrackRow> rows;
    const auto group = tracks_.FocusedGroup();
    for (const auto& track : tracks_.Rows())
    {
        const auto bank = std::find_if(track.banks.begin(), track.banks.end(),
            [group](const auto& bank) { return bank.group == group; });
        if (group && bank != track.banks.end())
        {
            rows.push_back({ track.id, bank->index, track.label });
        }
        else if (!group && track.id == tracks_.FocusedInstance())
        {
            rows.push_back({ track.id, tracks_.FocusedBank(), track.label });
        }
    }
    if (rows_ != rows)
    {
        rows_ = std::move(rows);
        list_.updateContent();
    }
    list_.repaint();
}

int GroupTrackList::getNumRows() { return static_cast<int>(rows_.size()); }

void GroupTrackList::paintListBoxItem(int row, juce::Graphics& graphics, int width, int height, bool)
{
    if (row < 0 || row >= getNumRows())
    {
        return;
    }
    const auto& track = rows_[static_cast<std::size_t>(row)];
    const bool focused = track.instance == tracks_.FocusedInstance();
    if (focused)
    {
        graphics.fillAll(shared::ui::kLines);
    }
    graphics.setColour(isEnabled() ? (focused ? shared::ui::kActiveText : shared::ui::kText) : shared::ui::kDisabled);
    graphics.setFont(shared::ui::ControlFont());
    graphics.drawText(juce::String(track.name), 6, 0, width - 12, height,
        juce::Justification::centredLeft);
}

void GroupTrackList::resized() { list_.setBounds(getLocalBounds()); }
void GroupTrackList::enablementChanged() { list_.repaint(); }

juce::String GroupTrackList::getTooltipForRow(int row)
{
    return row >= 0 && row < getNumRows() ? juce::String(rows_[static_cast<std::size_t>(row)].name) : juce::String{};
}

void GroupTrackList::listBoxItemClicked(int row, const juce::MouseEvent& event)
{
    if (!isEnabled() || !event.mods.isLeftButtonDown() || row < 0 || row >= getNumRows())
    {
        return;
    }
    const auto instance = rows_[static_cast<std::size_t>(row)].instance;
    const auto bank = rows_[static_cast<std::size_t>(row)].bank;
    emit_({ tracks::bindings::TrackIntentKind::SelectBank, instance, bank });
}

} // namespace consolidator::vst3::features::equalizer::controls
