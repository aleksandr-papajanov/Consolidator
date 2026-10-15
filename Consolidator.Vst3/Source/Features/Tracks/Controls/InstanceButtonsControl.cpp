#include "Features/Tracks/Controls/InstanceButtonsControl.h"

#include <algorithm>
#include <utility>

#include "Shared/Controls/ActionButton.h"
#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::tracks::controls
{

InstanceButtonsControl::InstanceButtonsControl(viewmodels::TracksViewModel& model,
    std::function<void(const bindings::TrackIntent&)> emit, std::uint64_t instance)
    : model_(model), emit_(std::move(emit)), instance_(instance)
{
    const std::array<const char*, 4> labels{ "S", "M", "R", "B" };
    const std::array<const char*, 4> tooltips{ "Solo (Shift: additive)", "Mute", "Reset", "Bypass" };
    for (std::size_t index = 0; index < buttons_.size(); ++index)
    {
        if (index == 2)
        {
            buttons_[index] = std::make_unique<shared::controls::ActionButton>(labels[index]);
        }
        else
        {
            buttons_[index] = std::make_unique<juce::TextButton>(labels[index]);
        }
        auto& button = *buttons_[index];
        button.setTriggeredOnMouseDown(true);
        button.getProperties().set(shared::ui::kAcceptedValueProperty, true);
        button.setTooltip(tooltips[index]);
        button.onClick = [this, index] { Click(index); };
        addAndMakeVisible(button);
    }
    RefreshPresentation();
}

void InstanceButtonsControl::SetInstance(std::uint64_t instance)
{
    instance_ = instance;
    RefreshPresentation();
}

void InstanceButtonsControl::SetVerticalConnections(bool above, bool below)
{
    verticalEdges_ = (above ? juce::Button::ConnectedOnTop : 0)
        | (below ? juce::Button::ConnectedOnBottom : 0);
    resized();
}

void InstanceButtonsControl::RefreshPresentation()
{
    const auto& rows = model_.Rows();
    const auto row = std::find_if(rows.begin(), rows.end(),
        [this](const auto& item) { return item.id == instance_; });
    const std::array<bool, 4> active{ row != rows.end() && row->solo,
        row != rows.end() && row->mute, false, row != rows.end() && row->bypass };
    const auto group = model_.FocusedGroup();
    const auto text = model_.GroupScope() && group ? shared::ui::GroupColor(*group) : shared::ui::kActiveText;
    for (std::size_t index = 0; index < buttons_.size(); ++index)
    {
        auto& button = *buttons_[index];
        button.setToggleState(active[index], juce::dontSendNotification);
        button.setColour(juce::TextButton::buttonOnColourId,
            index == 1 ? shared::ui::kMute : index >= 2 ? shared::ui::kReset : shared::ui::kActive);
        button.setColour(juce::TextButton::textColourOffId, text);
    }
}

void InstanceButtonsControl::Click(std::size_t index)
{
    const auto& rows = model_.Rows();
    const auto row = std::find_if(rows.begin(), rows.end(),
        [this](const auto& item) { return item.id == instance_; });
    if (!isEnabled() || row == rows.end())
    {
        return;
    }
    const std::array<bindings::TrackIntentKind, 4> kinds{
        bindings::TrackIntentKind::SetSolo, bindings::TrackIntentKind::SetMute,
        bindings::TrackIntentKind::ResetInstance, bindings::TrackIntentKind::SetInstanceBypass };
    const bool value = index == 0 ? !row->solo : index == 1 ? !row->mute : !row->bypass;
    emit_({ kinds[index], instance_, model_.FocusedBank(), {}, value,
        index != 0 || juce::ModifierKeys::getCurrentModifiersRealtime().isShiftDown() });
}

void InstanceButtonsControl::resized()
{
    for (std::size_t index = 0; index < buttons_.size(); ++index)
    {
        auto& button = *buttons_[index];
        button.setConnectedEdges(verticalEdges_ | (index > 0 ? juce::Button::ConnectedOnLeft : 0)
            | (index + 1 < buttons_.size() ? juce::Button::ConnectedOnRight : 0));
        button.setBounds(static_cast<int>(index) * shared::ui::kBankCellSize, 0,
            shared::ui::kBankCellSize, shared::ui::kBankCellSize);
    }
}

} // namespace consolidator::vst3::features::tracks::controls
