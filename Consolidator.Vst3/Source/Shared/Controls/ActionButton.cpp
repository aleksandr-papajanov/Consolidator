#include "Shared/Controls/ActionButton.h"

#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::shared::controls
{

ActionButton::ActionButton(const juce::String& label) : TextButton(label) { }
ActionButton::~ActionButton() { stopTimer(); }

void ActionButton::clicked()
{
    getProperties().set(shared::ui::kActionFlashProperty, true);
    startTimer(180);
    repaint();
}

void ActionButton::timerCallback()
{
    stopTimer();
    getProperties().set(shared::ui::kActionFlashProperty, false);
    repaint();
}

} // namespace consolidator::vst3::shared::controls
