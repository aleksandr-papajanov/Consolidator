#include "Shared/Controls/MarkerRowControl.h"

#include "Shared/Controls/MarkerControl.h"
#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::shared::controls
{

MarkerRowControl::MarkerRowControl(juce::String label) : juce::Button(label)
{
    setTriggeredOnMouseDown(true);
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
}

void MarkerRowControl::SetActive(bool active)
{
    setToggleState(active, juce::dontSendNotification);
}

void MarkerRowControl::paintButton(juce::Graphics& graphics, bool, bool)
{
    const bool active = getToggleState();
    const auto color = !isEnabled() ? ui::kDisabled : active ? ui::kActive : ui::kText;
    MarkerControl::DrawRow(graphics, getLocalBounds(), getButtonText(), color, active, 8, ui::kControlFontHeight);
}

} // namespace consolidator::vst3::shared::controls
