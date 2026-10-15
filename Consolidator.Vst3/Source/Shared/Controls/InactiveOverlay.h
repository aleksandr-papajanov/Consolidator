#pragma once

#include <JuceHeader.h>

namespace consolidator::vst3::shared::controls
{

class InactiveOverlay final : public juce::Component
{
public:
    InactiveOverlay();
    void paint(juce::Graphics& graphics) override;
    void mouseDown(const juce::MouseEvent& event) override;
    bool keyPressed(const juce::KeyPress& key) override;
    bool keyStateChanged(bool isKeyDown) override;
};

} // namespace consolidator::vst3::shared::controls
