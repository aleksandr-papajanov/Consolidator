#pragma once

#include <memory>
#include <string>

#include <JuceHeader.h>

#include "Shared/Controls/Dial/DialControl.h"
#include "Shared/Controls/Parameter/ParameterButton.h"
#include "Shared/Controls/Parameter/ChoiceDialControl.h"
#include "Shared/Editing/ParameterIntents.h"
#include "Shared/Controls/Parameter/ViewModels/ParameterViewModel.h"

namespace consolidator::vst3::shared::controls::parameter
{

class ParameterControl final : public juce::Component
{
public:
    ParameterControl(shared::controls::parameter::viewmodels::ParameterViewModel& model,
        shared::editing::ParameterIntents intents, juce::String buttonLabel, int connectedEdges);
    [[nodiscard]] const std::string& Path() const;
    [[nodiscard]] shared::controls::parameter::viewmodels::ParameterViewModel& Model();
    void Refresh();
    void SetScopeColor(juce::Colour color);
    void FinishPreview(bool accepted);
    void resized() override;

private:
    void UpdateButton();

    shared::controls::parameter::viewmodels::ParameterViewModel& model_;
    shared::editing::ParameterIntents intents_;
    std::unique_ptr<shared::controls::dial::DialControl> dial_;
    std::unique_ptr<ChoiceDialControl> choice_;
    ParameterButton button_;
    juce::String buttonLabel_;
};

} // namespace consolidator::vst3::shared::controls::parameter
