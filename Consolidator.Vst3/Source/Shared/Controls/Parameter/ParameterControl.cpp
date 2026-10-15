#include "Shared/Controls/Parameter/ParameterControl.h"

#include <cstdint>
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
    else if (model_.HasChoices())
    {
        choice_ = std::make_unique<ChoiceDialControl>(model_, intents_);
        addAndMakeVisible(*choice_);
    }
    else
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
    else if (choice_)
    {
        choice_->repaint();
    }
    else
    {
        UpdateButton();
    }
}

void ParameterControl::SetScopeColor(juce::Colour color)
{
    if (dial_)
    {
        dial_->SetScopeColor(color);
    }
    else if (choice_)
    {
        choice_->SetScopeColor(color);
    }
    else
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
    button_.setButtonText(buttonLabel_);
    button_.setToggleState(model_.ButtonValue(), juce::dontSendNotification);
    button_.setColour(juce::TextButton::textColourOffId, shared::ui::kActiveText);
}

void ParameterControl::resized()
{
    if (dial_)
    {
        dial_->setBounds(getLocalBounds());
    }
    else if (choice_)
    {
        choice_->setBounds(getLocalBounds());
    }
    else
    {
        button_.setBounds(getLocalBounds());
    }
}

} // namespace consolidator::vst3::shared::controls::parameter
