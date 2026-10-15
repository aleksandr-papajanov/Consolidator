#include "Shared/Controls/Parameter/ParameterControl.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <numbers>
#include <stdexcept>
#include <utility>

#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::shared::controls::parameter
{

ParameterControl::ParameterControl(shared::controls::parameter::viewmodels::ParameterViewModel& model,
    shared::editing::ParameterIntents intents, juce::String buttonLabel, int connectedEdges)
    : model_(model), intents_(std::move(intents)), buttonLabel_(std::move(buttonLabel))
{
    button_.setConnectedEdges(connectedEdges);
    button_.setTooltip(juce::String(model_.State().Label()));
    if (auto* dial = model_.Dial())
    {
        dial_ = std::make_unique<shared::controls::dial::DialControl>(*dial, intents_);
        addAndMakeVisible(*dial_);
    }
    else
    {
        if (!model_.HasChoices())
        {
            addAndMakeVisible(button_);
            button_.onClick = [this]
            {
                if (button_.ResetClick())
                {
                    intents_.reset(model_.State().Path());
                }
                else
                {
                    intents_.commit(model_.State().Path(), std::int64_t{ model_.ButtonValue() ? 0 : 1 });
                }
            };
        }
        UpdateButton();
    }
}

const std::string& ParameterControl::Path() const { return model_.State().Path(); }
shared::controls::parameter::viewmodels::ParameterViewModel& ParameterControl::Model() { return model_; }

void ParameterControl::Refresh()
{
    if (dial_)
    {
        dial_->Refresh();
    }
    else
    {
        UpdateButton();
    }
}

void ParameterControl::SetScopeColor(juce::Colour color)
{
    scopeColor_ = color;
    if (dial_)
    {
        dial_->SetScopeColor(color);
    }
    else if (!model_.HasChoices())
    {
        if (model_.GroupScope())
        {
            button_.getProperties().set(shared::ui::kGroupMarkerColorProperty,
                static_cast<juce::int64>(color.getARGB()));
        }
        else
        {
            button_.getProperties().remove(shared::ui::kGroupMarkerColorProperty);
        }
        button_.repaint();
    }
    repaint();
}

void ParameterControl::FinishPreview(bool accepted)
{
    if (dial_)
    {
        dial_->FinishPreview(accepted);
    }
}

void ParameterControl::UpdateButton()
{
    if (model_.HasChoices())
    {
        repaint();
        return;
    }
    else
    {
        button_.setButtonText(buttonLabel_);
        button_.setToggleState(model_.ButtonValue(), juce::dontSendNotification);
    }
    button_.setColour(juce::TextButton::textColourOffId, shared::ui::kActiveText);
}

void ParameterControl::paint(juce::Graphics& graphics)
{
    if (model_.HasChoices())
    {
        graphics.fillAll(shared::ui::kBackground);
        const int selected = choiceDragging_ ? dragChoice_ : model_.ChoiceValue();
        const juce::Point<float> center{ static_cast<float>(getWidth()) * 0.5F, static_cast<float>(getHeight()) * 0.55F };
        const auto indicator = ChoicePoint(selected) - center;
        const auto color = model_.GroupScope() ? scopeColor_ : shared::ui::kActiveText;
        graphics.setColour(color);
        graphics.drawLine({ center + indicator * 0.2F, center + indicator * 0.6F }, 2);
        for (std::size_t index = 0; index < model_.Choices().size(); ++index)
        {
            const auto point = ChoicePoint(static_cast<int>(index));
            graphics.setColour(static_cast<int>(index) == selected ? color : shared::ui::kLines);
            graphics.fillEllipse(point.x - 2, point.y - 2, 4, 4);
        }

        graphics.setColour(shared::ui::kText);
        graphics.setFont(shared::ui::ControlFont());
        const auto label = model_.Choices()[static_cast<std::size_t>(selected)];
        graphics.drawText(juce::String::fromUTF8(label.data(), static_cast<int>(label.size())), 0,
            getHeight() - 17, getWidth(), 14, juce::Justification::centred, true);
        return;
    }
}

juce::Point<float> ParameterControl::ChoicePoint(int index) const
{
    // Existing three-position toggle spans 120 degrees around the top of the dial.
    const double angle = std::numbers::pi * (7.0 / 6 + static_cast<double>(index) / 3);
    const float radius = std::max(1.0F, static_cast<float>(std::min(getWidth(), getHeight())) * 0.38F);
    return { static_cast<float>(getWidth()) * 0.5F + static_cast<float>(std::cos(angle)) * radius,
        static_cast<float>(getHeight()) * 0.55F + static_cast<float>(std::sin(angle)) * radius };
}

void ParameterControl::mouseDown(const juce::MouseEvent& event)
{
    if (model_.HasChoices() && isEnabled() && event.mods.isLeftButtonDown())
    {
        choiceDragging_ = true;
        dragChoice_ = model_.ChoiceValue();
        choicePosition_ = static_cast<double>(model_.ChoiceValue());
        lastChoiceY_ = event.position.y;
        repaint();
    }
}

void ParameterControl::mouseDrag(const juce::MouseEvent& event)
{
    if (!choiceDragging_ || !isEnabled())
    {
        return;
    }
    choicePosition_ += (lastChoiceY_ - event.position.y) * 0.015 * 2;
    lastChoiceY_ = event.position.y;
    const auto maximum = static_cast<double>(model_.Choices().size() - 1);
    const auto next = static_cast<std::int64_t>(std::clamp(std::floor(choicePosition_ + 0.5), 0.0, maximum));
    if (next != dragChoice_)
    {
        dragChoice_ = static_cast<int>(next);
        repaint();
        intents_.commit(model_.State().Path(), next);
    }
}

void ParameterControl::mouseUp(const juce::MouseEvent&)
{
    choiceDragging_ = false;
    dragChoice_ = -1;
    repaint();
}

void ParameterControl::resized()
{
    if (dial_)
    {
        dial_->setBounds(getLocalBounds());
    }
    else if (!model_.HasChoices())
    {
        button_.setBounds(getLocalBounds());
    }
}

} // namespace consolidator::vst3::shared::controls::parameter
