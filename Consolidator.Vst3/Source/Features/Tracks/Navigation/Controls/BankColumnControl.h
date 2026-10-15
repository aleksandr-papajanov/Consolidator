#pragma once

#include <functional>

#include <JuceHeader.h>

#include "Features/Tracks/Controllers/BankCellInteraction.h"

namespace consolidator::vst3::features::tracks::navigation::controls
{

class BankColumnControl final : public juce::Component
{
public:
    BankColumnControl(tracks::viewmodels::TracksViewModel& model,
        std::function<void(const tracks::bindings::TrackIntent&)> emit);
    void paint(juce::Graphics& graphics) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;

private:
    void SelectAt(const juce::MouseEvent& event, bool dragging);
    tracks::viewmodels::TracksViewModel& model_;
    tracks::controllers::BankCellInteraction interaction_;
};

} // namespace consolidator::vst3::features::tracks::navigation::controls
