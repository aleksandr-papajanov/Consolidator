#include "Shared/Controls/Parameter/ParameterButton.h"

#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::shared::controls::parameter
{

ParameterButton::ParameterButton()
{
    setTriggeredOnMouseDown(true);
    getProperties().set(shared::ui::kAcceptedValueProperty, true);
}
bool ParameterButton::ResetClick() const { return resetClick_; }

void ParameterButton::mouseDown(const juce::MouseEvent& event)
{
    resetClick_ = event.getNumberOfClicks() > 1;
    TextButton::mouseDown(event);
}

} // namespace consolidator::vst3::shared::controls::parameter
