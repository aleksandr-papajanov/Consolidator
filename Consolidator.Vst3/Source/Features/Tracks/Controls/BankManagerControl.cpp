#include "Features/Tracks/Controls/BankManagerControl.h"

#include <array>
#include <cstddef>
#include <utility>

#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::tracks::controls
{

BankManagerControl::BankManagerControl(viewmodels::TracksViewModel& tracks,
    std::function<void(const bindings::TrackIntent&)> emit, BankManagerActions actions)
    : tracks_(tracks), list_(tracks, std::move(emit))
{
    title_.setText("Consolidator", juce::dontSendNotification);
    title_.setFont(juce::Font(juce::FontOptions("Arial", shared::ui::kTitleFontHeight, juce::Font::plain)));
    title_.setColour(juce::Label::textColourId, shared::ui::kActiveText);
    group_.onClick = std::move(actions.group);
    ungroup_.onClick = std::move(actions.ungroup);
    clear_.onClick = std::move(actions.clear);
    reset_.onClick = std::move(actions.reset);
    for (auto* child : std::array<juce::Component*, 6>{ &title_, &group_, &ungroup_, &clear_, &reset_, &list_ })
    {
        addAndMakeVisible(child);
    }
}

void BankManagerControl::Refresh() { list_.Refresh(); }
void BankManagerControl::RefreshPresentation() { list_.RefreshPresentation(); }
void BankManagerControl::RefreshLevels() { list_.repaint(); }

void BankManagerControl::SetEditing(bool navigation, bool registryReady)
{
    list_.setEnabled(navigation && registryReady);
    const auto actions = tracks_.Actions();
    group_.setEnabled(navigation && actions.group);
    ungroup_.setEnabled(navigation && actions.ungroup);
    clear_.setEnabled(navigation && actions.clear);
    reset_.setEnabled(navigation);
}

void BankManagerControl::resized()
{
    const int tableWidth = getWidth() - 32;
    title_.setBounds(16, 4, 200, 36);
    const std::array<juce::Button*, 4> actions{ &group_, &ungroup_, &clear_, &reset_ };
    const int actionWidth = (tableWidth - 3 * shared::ui::kControlGap) / 4;
    for (std::size_t index = 0; index < actions.size(); ++index)
    {
        actions[index]->setBounds(16 + static_cast<int>(index) * (actionWidth + shared::ui::kControlGap),
            56, actionWidth, shared::ui::kButtonHeight);
    }
    list_.setBounds(16, 94, tableWidth, getHeight() - 110);
}

} // namespace consolidator::vst3::features::tracks::controls
