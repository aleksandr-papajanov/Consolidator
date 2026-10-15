#include "Shared/Controls/InactiveOverlay.h"

#include "Shared/Theme/UiColors.h"

namespace consolidator::vst3::shared::controls
{

InactiveOverlay::InactiveOverlay()
{
    setInterceptsMouseClicks(true, true);
    setWantsKeyboardFocus(true);
    setAlwaysOnTop(true);
}

void InactiveOverlay::paint(juce::Graphics& graphics)
{
    graphics.fillAll(shared::ui::kBackground.withAlpha(0.65F));
    graphics.setColour(shared::ui::kActiveText);
    graphics.setFont(juce::Font(juce::FontOptions(32.0F)));
    graphics.drawText("Inactive", getLocalBounds(), juce::Justification::centred);
}

void InactiveOverlay::mouseDown(const juce::MouseEvent&)
{
    // The first click focuses the window but never reaches a control below.
    grabKeyboardFocus();
}

bool InactiveOverlay::keyPressed(const juce::KeyPress&)
{
    return true;
}

bool InactiveOverlay::keyStateChanged(bool)
{
    return true;
}

} // namespace consolidator::vst3::shared::controls
