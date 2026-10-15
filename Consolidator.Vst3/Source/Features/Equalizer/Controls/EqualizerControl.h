#pragma once

#include <cstdint>
#include <functional>
#include <string>

#include <JuceHeader.h>

#include "Features/Equalizer/Bindings/AnalyzerBindings.h"
#include "Features/Equalizer/Controls/AnalyzerControl.h"
#include "Features/Equalizer/Controls/AnalyzerSourceList.h"
#include "Features/Equalizer/Controls/AnalyzerOverlayList.h"
#include "Features/Tracks/Navigation/Controls/TrackNavigationControl.h"
#include "Shared/Controls/ActionButton.h"
#include "Shared/Controls/MarkerControl.h"
#include "Shared/Clients/RegistryClient.h"

namespace consolidator::vst3::features::equalizer::controls
{

struct EqualizerActions
{
    std::function<void(const tracks::bindings::TrackIntent&)> trackIntent;
    std::function<void(const std::string&, std::int64_t)> discreteValue;
    std::function<void(const std::string&)> resetValue;
    std::function<void(const std::string&)> processBank;
    std::function<double()> sampleRate;
};

// Owns the entire fixed EQ section; target selection remains shared with Tracks.
class EqualizerControl final : public juce::Component
{
public:
    EqualizerControl(bindings::AnalyzerBindings& bindings, const shared::clients::TargetStateClient& state,
        tracks::viewmodels::TracksViewModel& tracks, EqualizerActions actions);
    [[nodiscard]] AnalyzerControl& Graph();
    [[nodiscard]] const std::string& ContextPath() const;
    void ApplySnapshot();
    void RefreshAcceptedState();
    void RefreshTrackLevels();
    void ApplyProcessorStatus(const shared::clients::RegistryProcessor& processor);
    void SetEditingState(bool navigation, bool graphEditing, bool banksReady);
    void paint(juce::Graphics& graphics) override;
    void resized() override;

private:
    bindings::AnalyzerBindings& bindings_;
    const shared::clients::TargetStateClient& state_;
    tracks::viewmodels::TracksViewModel& tracks_;
    EqualizerActions actions_;
    AnalyzerControl graph_;
    tracks::navigation::controls::TrackNavigationControl navigation_;
    AnalyzerSourceList sources_;
    AnalyzerOverlayList overlays_;
    shared::controls::MarkerControl processorMarker_;
    shared::controls::ActionButton commit_{ "Commit" };
    shared::controls::ActionButton refine_{ "Refine" };
};

} // namespace consolidator::vst3::features::equalizer::controls
