#include "Features/Equalizer/Controls/EqualizerControl.h"

#include <array>
#include <utility>

#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::equalizer::controls
{

EqualizerControl::EqualizerControl(bindings::AnalyzerBindings& bindings,
    const shared::clients::TargetStateClient& state, tracks::viewmodels::TracksViewModel& tracks,
    EqualizerActions actions)
    : bindings_(bindings), state_(state), tracks_(tracks), actions_(std::move(actions)),
      graph_(bindings.Model(), bindings.Intents()), banks_(tracks, actions_.trackIntent),
      groupTracks_(tracks, actions_.trackIntent), sources_([this] { ApplySnapshot(); }),
      overlays_([this](bool reference, bool group, bool allBanks) { graph_.SetOverlays(reference, group, allBanks); })
{
    setOpaque(true);
    bypass_.getProperties().set(shared::ui::kAcceptedValueProperty, true);
    bypass_.setTooltip("Equalizer Bypass");
    bypass_.onClick = [this]
    {
        actions_.discreteValue("equalizer.bypass",
            shared::protocol::Boolean(state_.Require("equalizer.bypass").value) ? 0 : 1);
    };
    commit_.onClick = [this] { actions_.processBank("commit_bank"); };
    refine_.onClick = [this] { actions_.processBank("refine_bank"); };
    bypass_.setConnectedEdges(juce::Button::ConnectedOnRight);
    commit_.setConnectedEdges(juce::Button::ConnectedOnLeft | juce::Button::ConnectedOnRight);
    refine_.setConnectedEdges(juce::Button::ConnectedOnLeft);
    for (auto* component : std::array<juce::Component*, 8>{
        &graph_, &banks_, &groupTracks_, &sources_, &overlays_, &bypass_, &commit_, &refine_ })
    {
        addAndMakeVisible(component);
    }
}

AnalyzerControl& EqualizerControl::Graph() { return graph_; }

const std::string& EqualizerControl::ContextPath() const
{
    return sources_.ContextPath();
}

void EqualizerControl::ApplySnapshot()
{
    if (!state_.Ready())
    {
        return;
    }
    graph_.ResetInteraction();
    bindings_.SetSnapshot(ContextPath(), state_.Values(), actions_.sampleRate());
    graph_.Refresh();
    RefreshAcceptedState();
}

void EqualizerControl::RefreshAcceptedState()
{
    if (state_.Ready())
    {
        bypass_.setToggleState(shared::protocol::Boolean(state_.Require("equalizer.bypass").value),
            juce::dontSendNotification);
    }
    bindings_.SetScope(tracks_.GroupScope());
    graph_.SetGroupScope(tracks_.GroupScope());
    const auto* bank = tracks_.FindBank(tracks_.FocusedInstance(), tracks_.FocusedBank());
    graph_.SetGroupColor(bank != nullptr && bank->group
        ? shared::ui::GroupColor(*bank->group) : shared::ui::kLines);
    banks_.repaint();
    groupTracks_.Refresh();
}

void EqualizerControl::SetEditingState(bool navigation, bool graphEditing, bool banksReady)
{
    sources_.setEnabled(navigation);
    overlays_.setEnabled(navigation);
    bypass_.setEnabled(navigation);
    graph_.setEnabled(graphEditing);
    banks_.setEnabled(navigation && banksReady);
    groupTracks_.setEnabled(navigation && banksReady);
    const bool processing = navigation && tracks_.Actions().process;
    commit_.setEnabled(processing);
    refine_.setEnabled(processing);
    RefreshAcceptedState();
}

void EqualizerControl::resized()
{
    const int bodyHeight = getHeight() - shared::ui::kButtonHeight;
    const int bankWidth = shared::ui::kBankCellSize + 1;
    constexpr int headerHeight = 32;
    const int contentHeight = bodyHeight - headerHeight;
    constexpr int groupWidth = 136;
    constexpr int sourceWidth = 112;
    banks_.setBounds(0, headerHeight, bankWidth, contentHeight);
    groupTracks_.setBounds(bankWidth, headerHeight, groupWidth, contentHeight);
    graph_.setBounds(bankWidth + groupWidth, headerHeight,
        getWidth() - bankWidth - groupWidth - sourceWidth, contentHeight);
    sources_.setBounds(getWidth() - sourceWidth, headerHeight, sourceWidth, 4 * shared::ui::kBankCellSize);
    overlays_.setBounds(getWidth() - sourceWidth, sources_.getBottom(), sourceWidth, 3 * shared::ui::kBankCellSize);
    constexpr int actionWidth = 28 + 88 + 88;
    const int actionLeft = (getWidth() - actionWidth) / 2;
    bypass_.setBounds(actionLeft, bodyHeight, 28, shared::ui::kButtonHeight);
    commit_.setBounds(actionLeft + 28, bodyHeight, 88, shared::ui::kButtonHeight);
    refine_.setBounds(actionLeft + 116, bodyHeight, 88, shared::ui::kButtonHeight);
}

void EqualizerControl::paint(juce::Graphics& graphics)
{
    graphics.fillAll(shared::ui::kAnalyzerBackground);
    graphics.setColour(shared::ui::kActiveText);
    graphics.setFont(juce::Font(juce::FontOptions("Arial", shared::ui::kSectionFontHeight, juce::Font::plain)));
    graphics.drawText("EQUALIZER", graph_.getX(), 4, graph_.getWidth(), 28, juce::Justification::centred);
}

} // namespace consolidator::vst3::features::equalizer::controls
