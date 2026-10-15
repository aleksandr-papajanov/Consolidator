#pragma once

#include <JuceHeader.h>

namespace consolidator::vst3::shared::controls::parameter
{

class ParameterButton final : public juce::TextButton
{
public:
    ParameterButton();
    void mouseDown(const juce::MouseEvent& event) override;
    [[nodiscard]] bool ResetClick() const;

private:
    bool resetClick_{};
};

} // namespace consolidator::vst3::shared::controls::parameter
