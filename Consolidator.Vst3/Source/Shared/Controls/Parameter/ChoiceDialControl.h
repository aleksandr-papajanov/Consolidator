#pragma once

#include <JuceHeader.h>

#include "Shared/Controls/Parameter/ViewModels/ParameterViewModel.h"
#include "Shared/Editing/ParameterIntents.h"

namespace consolidator::vst3::shared::controls::parameter
{

class ChoiceDialControl final : public juce::Component
{
public:
    ChoiceDialControl(viewmodels::ParameterViewModel& model, shared::editing::ParameterIntents intents);
    void SetScopeColor(juce::Colour color);
    void paint(juce::Graphics& graphics) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    [[nodiscard]] juce::Point<float> ChoicePoint(double index, int count) const;
    viewmodels::ParameterViewModel& model_;
    shared::editing::ParameterIntents intents_;
    juce::Colour scopeColor_;
    bool choiceDragging_{};
    double choicePosition_{};
    float lastChoiceY_{};
    int dragChoice_{ -1 };
};

} // namespace consolidator::vst3::shared::controls::parameter
