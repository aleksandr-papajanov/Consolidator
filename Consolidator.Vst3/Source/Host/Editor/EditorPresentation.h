#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <string>
#include <vector>

#include <JuceHeader.h>

#include "Host/Editor/EditorServices.h"
#include "Shared/Theme/UiLookAndFeel.h"
#include "Features/Equalizer/Controls/EqualizerControl.h"
#include "Features/Tracks/Controls/BankManagerControl.h"
#include "Features/Equalizer/Controllers/EqualizerController.h"
#include "Features/Tracks/Controls/InstanceButtonsControl.h"
#include "Features/Tracks/Controllers/TrackController.h"
#include "Shared/Editing/DialGestureSession.h"
#include "Shared/Controls/InactiveOverlay.h"
#include "Shared/Controls/ActionButton.h"
#include "Features/ChannelStrip/Controls/ChannelStripControl.h"
#include "Shared/Clients/RegistryClient.h"
#include "Shared/Clients/TrackLevelClient.h"
#include "Shared/Clients/ProtocolClient.h"
#include "Shared/Clients/TargetStateClient.h"
#include "Shared/Clients/StateCommands.h"
#include "Shared/Clients/HistoryClient.h"
#include "Features/ChannelStrip/GainStaging/Clients/InputTelemetryClient.h"
#include "Features/ChannelStrip/Bindings/ChannelStripBindings.h"
#include "Features/Equalizer/Clients/AnalyzerClient.h"
#include "Features/Equalizer/Bindings/AnalyzerBindings.h"
#include "Host/Editor/EditorSession.h"
#include "Features/Tracks/ViewModels/TracksViewModel.h"

namespace consolidator::vst3::host::editor
{

class EditorPresentation final : public juce::Component
{
public:
    explicit EditorPresentation(EditorServices services);
    ~EditorPresentation() override;
    void Start();
    [[nodiscard]] int LogicalWidth() const;
    void HandleOutput(const shared::protocol::ProtocolFrame& frame);
    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    using Completion = std::function<void(bool)>;
    void RouteOutput(const shared::protocol::ProtocolFrame& frame);
    void Observe();
    void ApplySnapshot(const shared::protocol::ProtocolFrame& frame);
    void ApplyChangedValue(const shared::protocol::ProtocolFrame& frame);
    void ApplyTelemetry(const shared::protocol::ProtocolFrame& frame);
    void ResetValue(const std::string& path);
    void FetchRegistry();
    void FetchBankSnapshots();
    void RegistryChanged();
    void RefreshProcessorMarkers();
    void HandleTrackIntent(const features::tracks::bindings::TrackIntent& intent);
    void SelectTarget(std::uint64_t instance, int bank);
    void GroupSelectedBanks();
    void UngroupFocusedBank();
    void ClearGroups();
    void ProcessBank(const std::string& selector);
    bool BeginGesture(const std::string& path, std::vector<shared::protocol::StateWriteEntry> values, bool analyzerGesture);
    void GestureFinished(bool accepted);
    void SetGroupScope(bool groupScope);
    void CommitDiscreteValue(const std::string& path, std::int64_t value);
    void ResetAll();
    void SetBusy(bool busy);
    void UpdateInactiveOverlay();
    void RefreshTrackTitle();
    void PositionTrackHeader();
    [[nodiscard]] Completion CommandCompletion();
    [[nodiscard]] std::string Scope() const;

    // Declared first so the theme outlives every child borrowing it.
    shared::ui::UiLookAndFeel lookAndFeel_;
    EditorServices services_;
    shared::clients::ProtocolClient commands_;
    shared::clients::TargetStateClient targetState_;
    shared::clients::StateCommands stateCommands_;
    features::equalizer::clients::AnalyzerClient analyzerClient_;
    shared::clients::HistoryClient history_;
    features::channelstrip::gainstaging::clients::InputTelemetryClient telemetry_;
    shared::clients::RegistryClient registry_;
    shared::clients::TrackLevelClient trackLevels_;
    std::uint64_t nextTransaction_{ 1 };
    features::tracks::controllers::TrackController trackActions_;
    features::equalizer::controllers::EqualizerController equalizerActions_;
    features::tracks::viewmodels::TracksViewModel tracks_;
    features::tracks::controls::BankManagerControl bankManager_;
    features::tracks::controls::InstanceButtonsControl instanceButtons_;
    shared::editing::DialGestureSession gesture_;
    features::equalizer::bindings::AnalyzerBindings analyzerBindings_;
    features::equalizer::controls::EqualizerControl equalizer_;
    features::channelstrip::bindings::ChannelStripBindings channelStripBindings_;
    features::channelstrip::controls::ChannelStripControl channelStrip_;
    EditorSession session_;
    juce::Label trackTitle_;
    juce::Label status_;
    shared::controls::ActionButton undo_{ "Undo" };
    shared::controls::ActionButton redo_{ "Redo" };
    juce::TextButton scope_{ "Local" };
    juce::TextButton scale_{ "100%" };
    juce::TextButton trackListToggle_{ "Hide Tracks" };
    shared::controls::InactiveOverlay inactiveOverlay_;
    bool busy_{ true };
    bool closing_{};
    bool analyzerGesture_{};
    bool trackListExpanded_{ true };

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(EditorPresentation)
};
} // namespace consolidator::vst3::host::editor
