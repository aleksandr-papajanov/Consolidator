#pragma once

#include <functional>

#include <JuceHeader.h>

#include "Features/Tracks/Controls/TrackListControl.h"
#include "Shared/Controls/ActionButton.h"

namespace consolidator::vst3::features::tracks::controls
{

struct BankManagerActions
{
    std::function<void()> group;
    std::function<void()> ungroup;
    std::function<void()> clear;
    std::function<void()> reset;
};

class BankManagerControl final : public juce::Component
{
public:
    BankManagerControl(viewmodels::TracksViewModel& tracks,
        std::function<void(const bindings::TrackIntent&)> emit, BankManagerActions actions);
    void Refresh();
    void RefreshPresentation();
    void RefreshLevels();
    void SetEditing(bool navigation, bool registryReady);
    void resized() override;

private:
    viewmodels::TracksViewModel& tracks_;
    juce::Label title_;
    shared::controls::ActionButton group_{ "Group" };
    shared::controls::ActionButton ungroup_{ "Ungroup" };
    shared::controls::ActionButton clear_{ "Clear" };
    juce::TextButton reset_{ "Reset All" };
    TrackListControl list_;
};

} // namespace consolidator::vst3::features::tracks::controls
