#include "Shared/Controls/Dial/DialControl.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>
#include <utility>

#include "Shared/Controls/Dial/DialGeometry.h"
#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::shared::controls::dial
{

DialControl::DialControl(shared::controls::dial::DialViewModel& model, shared::editing::ParameterIntents intents)
    : model_(model), intents_(std::move(intents))
{
}

DialControl::~DialControl()
{
    stopTimer();
}

void DialControl::Refresh()
{
    repaint();
}

void DialControl::SetScopeColor(juce::Colour color)
{
    scopeColor_ = color;
    repaint();
}

void DialControl::FinishPreview(bool accepted)
{
    dragging_ = false;
    if (!accepted)
    {
        stopTimer();
        showValue_ = false;
    }
    repaint();
}

void DialControl::DrawArc(juce::Graphics& graphics, juce::Point<float> center, float radius,
    double start, double end, juce::Colour color, float thickness) const
{
    if (radius <= 0 || end <= start)
    {
        return;
    }
    juce::Path arc;
    arc.addCentredArc(center.x, center.y, radius, radius, 0, DialGeometry::Angle(start), DialGeometry::Angle(end), true);
    graphics.setColour(color);
    graphics.strokePath(arc, juce::PathStrokeType(thickness));
}

void DialControl::paint(juce::Graphics& graphics)
{
    graphics.fillAll(shared::ui::kBackground);
    const auto geometry = DialGeometry::FromBounds(getLocalBounds());
    const auto center = geometry.center;
    const auto radius = geometry.radius;
    const auto [minimum, maximum] = model_.RingRange();
    const auto color = !model_.State().Available() ? shared::ui::kDisabled :
        model_.GroupScope() ? scopeColor_ :
        model_.Automated() ? shared::ui::kDisabled : shared::ui::kActiveText;
    DrawArc(graphics, center, radius, minimum, maximum, shared::ui::kLines, 3);
    DrawArc(graphics, center, radius, minimum, model_.RingValue(), color, 3);
    if (const auto fast = model_.FastPosition())
    {
        DrawArc(graphics, center, radius - 6, std::min(0.5, *fast), std::max(0.5, *fast),
            model_.GroupScope() ? scopeColor_ : shared::ui::kActive, 3);
    }
    graphics.setColour(model_.State().Available() ? shared::ui::kText : shared::ui::kDisabled);
    graphics.setFont(shared::ui::ControlFont());
    const juce::String text(showValue_ ? model_.ValueText() : model_.State().Label());
    graphics.drawText(text, geometry.label, juce::Justification::centred, true);
}

void DialControl::mouseDown(const juce::MouseEvent& event)
{
    if (!model_.Editable() || !isEnabled() || dragging_ || model_.Previewing() || !event.mods.isLeftButtonDown())
    {
        return;
    }
    if (event.getNumberOfClicks() > 1)
    {
        intents_.reset(model_.State().Path());
        return;
    }
    const double value = shared::protocol::Number(model_.State().Entry().value);
    if (!intents_.begin(model_.State().Path(), value))
    {
        return;
    }
    model_.SetPreview(value);
    gesturePosition_ = model_.Normalize(value);
    lastY_ = event.position.y;
    dragging_ = true;
    showValue_ = true;
    stopTimer();
    repaint();
}

void DialControl::mouseDrag(const juce::MouseEvent& event)
{
    if (!dragging_ || !model_.Editable() || !isEnabled())
    {
        return;
    }
    const auto range = model_.EditingRange();
    gesturePosition_ = std::clamp(gesturePosition_ + (lastY_ - event.position.y) * 0.007,
        model_.Normalize(range.first), model_.Normalize(range.second));
    lastY_ = event.position.y;
    const double next = model_.Quantize(gesturePosition_, event.mods.isShiftDown());
    if (!model_.Previewing() || next != model_.DisplayValue())
    {
        model_.SetPreview(next);
        repaint();
        intents_.change(next);
    }
}

void DialControl::mouseUp(const juce::MouseEvent&)
{
    if (dragging_)
    {
        dragging_ = false;
        const double value = model_.DisplayValue();
        startTimer(500);
        intents_.end(value);
    }
}

void DialControl::timerCallback()
{
    stopTimer();
    showValue_ = false;
    repaint();
}

} // namespace consolidator::vst3::shared::controls::dial
