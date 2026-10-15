#include "Features/Tracks/Navigation/Controls/GroupSelectorControl.h"

#include <cstddef>
#include <utility>

#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::tracks::navigation::controls
{

GroupSelectorControl::GroupSelectorControl(const tracks::viewmodels::TracksViewModel& tracks,
    std::function<void(std::uint32_t)> select)
    : tracks_(tracks), select_(std::move(select))
{
    for (std::size_t index = 0; index < buttons_.size(); ++index)
    {
        auto& button = buttons_[index];
        const auto group = static_cast<std::uint32_t>(index);
        button.setButtonText(juce::String(tracks::viewmodels::TracksViewModel::GroupLabel(group)));
        button.getProperties().set(shared::ui::kAcceptedValueProperty, true);
        button.setConnectedEdges((index > 0 ? juce::Button::ConnectedOnLeft : 0) |
            (index + 1 < buttons_.size() ? juce::Button::ConnectedOnRight : 0));
        button.onClick = [this, group] { select_(group); };
        button.setEnabled(false);
        addAndMakeVisible(button);
    }
}

void GroupSelectorControl::Refresh(std::optional<std::uint32_t> selected)
{
    const auto sizes = tracks_.GroupSizes();
    for (std::size_t index = 0; index < buttons_.size(); ++index)
    {
        const auto group = static_cast<std::uint32_t>(index);
        const auto size = sizes.find(group);
        const bool exists = size != sizes.end();
        auto& button = buttons_[index];
        const auto color = shared::ui::GroupColor(group);
        button.setEnabled(exists);
        button.setToggleState(exists && selected == group, juce::dontSendNotification);
        button.setColour(juce::TextButton::buttonOnColourId, color);
        button.setColour(juce::TextButton::textColourOffId, exists ? color : shared::ui::kDisabled);
        const auto description = exists
            ? juce::String(static_cast<int>(size->second)) + " tracks" : juce::String("empty");
        button.setTooltip(juce::String("Group ") + button.getButtonText() + ": " + description +
            (group == 0 ? " (automatic)" : ""));
    }
}

int GroupSelectorControl::PreferredWidth() const
{
    return static_cast<int>(buttons_.size()) * shared::ui::kBankCellSize;
}

void GroupSelectorControl::resized()
{
    for (std::size_t index = 0; index < buttons_.size(); ++index)
    {
        buttons_[index].setBounds(static_cast<int>(index) * shared::ui::kBankCellSize, 0,
            shared::ui::kBankCellSize, shared::ui::kBankCellSize);
    }
}

} // namespace consolidator::vst3::features::tracks::navigation::controls
