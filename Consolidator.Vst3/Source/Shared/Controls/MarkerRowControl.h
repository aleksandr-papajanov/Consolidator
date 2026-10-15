#pragma once

#include <JuceHeader.h>

namespace consolidator::vst3::shared::controls
{

// A compact clickable row; selection policy belongs to its feature.
class MarkerRowControl final : public juce::Button
{
public:
    explicit MarkerRowControl(juce::String label);
    void SetActive(bool active);
    void paintButton(juce::Graphics& graphics, bool highlighted, bool down) override;
};

} // namespace consolidator::vst3::shared::controls
