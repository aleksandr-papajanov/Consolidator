#include "Shared/Controls/Parameter/ChoiceDialControl.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <stdexcept>
#include <utility>

#include "Shared/Controls/Dial/DialGeometry.h"
#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::shared::controls::parameter
{

ChoiceDialControl::ChoiceDialControl(viewmodels::ParameterViewModel& model, shared::editing::ParameterIntents intents)
    : model_(model), intents_(std::move(intents))
{
}

void ChoiceDialControl::SetScopeColor(juce::Colour color)
{
    scopeColor_ = color;
    repaint();
}

void ChoiceDialControl::paint(juce::Graphics& graphics)
{
    graphics.fillAll(shared::ui::kBackground);
    const int selected = choiceDragging_ ? dragChoice_ : model_.ChoiceValue();
    const auto geometry = dial::DialGeometry::FromBounds(getLocalBounds());
    const auto center = geometry.center;
    const auto count = static_cast<int>(model_.Choices().size());
    const auto indicator = ChoicePoint(selected, count) - center;
    const auto color = model_.GroupScope() ? scopeColor_ : shared::ui::kActiveText;
    juce::Path ring;
    ring.startNewSubPath(ChoicePoint(0, count));
    constexpr int kSegments = 32;
    for (int segment = 1; segment <= kSegments; ++segment)
    {
        ring.lineTo(ChoicePoint(static_cast<double>(count - 1) * segment / kSegments, count));
    }
    graphics.setColour(shared::ui::kLines);
    graphics.strokePath(ring, juce::PathStrokeType(3));
    graphics.setColour(color);
    graphics.drawLine({ center + indicator * 0.2F, center + indicator * 0.78F }, 2);
    for (std::size_t index = 0; index < model_.Choices().size(); ++index)
    {
        const auto point = ChoicePoint(static_cast<int>(index), count);
        graphics.setColour(static_cast<int>(index) == selected ? color : shared::ui::kText);
        graphics.fillEllipse(point.x - 2, point.y - 2, 4, 4);
    }

    graphics.setColour(shared::ui::kText);
    graphics.setFont(shared::ui::ControlFont());
    const auto label = model_.Choices()[static_cast<std::size_t>(selected)];
    const auto text = juce::String(model_.State().Label()) + ": " +
        juce::String::fromUTF8(label.data(), static_cast<int>(label.size()));
    graphics.drawText(text, geometry.label, juce::Justification::centred, true);
}

juce::Point<float> ChoiceDialControl::ChoicePoint(double index, int count) const
{
    if (count <= 0 || index < 0 || index >= count)
    {
        throw std::out_of_range("Choice point index is out of range.");
    }
    const auto geometry = dial::DialGeometry::FromBounds(getLocalBounds());
    return geometry.Point(count > 1 ? index / static_cast<double>(count - 1) : 0.5);
}

void ChoiceDialControl::mouseDown(const juce::MouseEvent& event)
{
    if (!isEnabled() || !event.mods.isLeftButtonDown())
    {
        return;
    }
    choiceDragging_ = true;
    dragChoice_ = model_.ChoiceValue();
    choicePosition_ = static_cast<double>(model_.ChoiceValue());
    lastChoiceY_ = event.position.y;
    repaint();
}

void ChoiceDialControl::mouseDrag(const juce::MouseEvent& event)
{
    if (!choiceDragging_ || !isEnabled())
    {
        return;
    }
    const auto maximum = static_cast<double>(model_.Choices().size() - 1);
    choicePosition_ += (lastChoiceY_ - event.position.y) * 0.015 * maximum;
    lastChoiceY_ = event.position.y;
    const auto next = static_cast<std::int64_t>(std::clamp(std::floor(choicePosition_ + 0.5), 0.0, maximum));
    if (next != dragChoice_)
    {
        dragChoice_ = static_cast<int>(next);
        repaint();
        intents_.commit(model_.State().Path(), next);
    }
}

void ChoiceDialControl::mouseUp(const juce::MouseEvent&)
{
    choiceDragging_ = false;
    dragChoice_ = -1;
    repaint();
}

} // namespace consolidator::vst3::shared::controls::parameter
