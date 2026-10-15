#include "Features/Equalizer/Controls/EqualizerControl.h"

#include <array>
#include <utility>

#include "Shared/Presentation/ProcessorPresentation.h"
#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::equalizer::controls
{

EqualizerControl::EqualizerControl(bindings::AnalyzerBindings& bindings,
    const shared::clients::TargetStateClient& state, tracks::viewmodels::TracksViewModel& tracks,
    EqualizerActions actions)
    : bindings_(bindings), state_(state), tracks_(tracks), actions_(std::move(actions)),
      graph_(bindings.Model(), bindings.Intents()), navigation_(tracks, actions_.trackIntent),
      sources_([this] { ApplySnapshot(); }),
      overlays_([this](bool reference, bool group, bool allBanks) { graph_.SetOverlays(reference, group, allBanks); }),
      processorMarker_(shared::presentation::ProcessorFor("equalizer").label.data(),
          shared::presentation::ProcessorFor("equalizer").color)
{
    setOpaque(true);
    processorMarker_.onToggle = [this]
    {
        actions_.discreteValue("equalizer.bypass",
            shared::protocol::Boolean(state_.Require("equalizer.bypass").value) ? 0 : 1);
    };
    processorMarker_.onReset = [this] { actions_.resetValue("equalizer"); };
    commit_.onClick = [this] { actions_.processBank("commit_bank"); };
    refine_.onClick = [this] { actions_.processBank("refine_bank"); };
    for (auto* component : std::array<juce::Component*, 7>{
        &graph_, &navigation_, &sources_, &overlays_, &processorMarker_, &commit_, &refine_ })
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
    bindings_.SetScope(tracks_.GroupScope());
    graph_.SetGroupScope(tracks_.GroupScope());
    const auto* bank = tracks_.FindBank(tracks_.FocusedInstance(), tracks_.FocusedBank());
    graph_.SetGroupColor(bank != nullptr && bank->group
        ? shared::ui::GroupColor(*bank->group) : shared::ui::kLines);
    navigation_.Refresh();
}

void EqualizerControl::RefreshTrackLevels() { navigation_.RefreshLevels(); }

void EqualizerControl::SetEditingState(bool navigation, bool graphEditing, bool banksReady)
{
    sources_.setEnabled(navigation);
    overlays_.setEnabled(navigation);
    processorMarker_.setEnabled(navigation);
    graph_.setEnabled(graphEditing);
    navigation_.setEnabled(navigation && banksReady);
    const bool processing = navigation && tracks_.Actions().process;
    commit_.setEnabled(processing);
    refine_.setEnabled(processing);
    RefreshAcceptedState();
}

void EqualizerControl::ApplyProcessorStatus(const shared::clients::RegistryProcessor& processor)
{
    processorMarker_.SetProcessorState(processor.active, processor.bypass);
}

void EqualizerControl::resized()
{
    const int bodyHeight = getHeight();
    constexpr int headerHeight = 32;
    const int contentTop = headerHeight + shared::ui::kBankCellSize;
    const int contentHeight = bodyHeight - contentTop;
    constexpr int sourceWidth = 112;
    navigation_.setBounds(0, headerHeight, getWidth() - sourceWidth, bodyHeight - headerHeight);
    graph_.setBounds(navigation_.ContentWidth(), contentTop,
        getWidth() - navigation_.ContentWidth() - sourceWidth, contentHeight);
    sources_.setBounds(getWidth() - sourceWidth, contentTop, sourceWidth, 5 * shared::ui::kBankCellSize);
    overlays_.setBounds(getWidth() - sourceWidth, sources_.getBottom(), sourceWidth, 3 * shared::ui::kBankCellSize);
    const int center = graph_.getBounds().getCentreX();
    const int markerWidth = processorMarker_.PreferredWidth();
    processorMarker_.setBounds(center - markerWidth / 2, 4, markerWidth, 28);
    const int actionLeft = getWidth() - sourceWidth + (sourceWidth - 88) / 2;
    const int actionTop = getHeight() - 16 - 2 * shared::ui::kButtonHeight - shared::ui::kControlGap;
    commit_.setBounds(actionLeft, actionTop, 88, shared::ui::kButtonHeight);
    refine_.setBounds(actionLeft, actionTop + shared::ui::kButtonHeight + shared::ui::kControlGap,
        88, shared::ui::kButtonHeight);
}

void EqualizerControl::paint(juce::Graphics& graphics)
{
    graphics.fillAll(shared::ui::kAnalyzerBackground);
}

} // namespace consolidator::vst3::features::equalizer::controls
