#pragma once

#include <memory>
#include <string>

#include <JuceHeader.h>

#include "Shared/Controls/Dial/DialControl.h"
#include "Shared/Controls/Parameter/ParameterButton.h"
#include "Shared/Editing/ParameterIntents.h"
#include "Shared/Controls/Parameter/ViewModels/ParameterViewModel.h"

namespace consolidator::vst3::shared::controls::parameter
{

class ParameterControl final : public juce::Component
{
public:
    ParameterControl(shared::controls::parameter::viewmodels::ParameterViewModel& model,
        shared::editing::ParameterIntents intents);
    [[nodiscard]] const std::string& Path() const;
    [[nodiscard]] shared::controls::parameter::viewmodels::ParameterViewModel& Model();
    void Refresh();
    void SetScopeColor(juce::Colour color);
    void FinishPreview(bool accepted);
    void paint(juce::Graphics& graphics) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    void UpdateButton();
    [[nodiscard]] juce::Point<float> ChoicePoint(int index) const;

    shared::controls::parameter::viewmodels::ParameterViewModel& model_;
    shared::editing::ParameterIntents intents_;
    std::unique_ptr<shared::controls::dial::DialControl> dial_;
    ParameterButton button_;
    juce::Colour scopeColor_;
    bool choiceDragging_{};
    double choicePosition_{};
    float lastChoiceY_{};
    int dragChoice_{ -1 };
};

} // namespace consolidator::vst3::shared::controls::parameter
