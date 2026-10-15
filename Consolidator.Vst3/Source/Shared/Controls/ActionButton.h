#pragma once

#include <JuceHeader.h>

namespace consolidator::vst3::shared::controls
{

class ActionButton final : public juce::TextButton, private juce::Timer
{
public:
    explicit ActionButton(const juce::String& label);
    ~ActionButton() override;

private:
    void clicked() override;
    void timerCallback() override;
};

} // namespace consolidator::vst3::shared::controls
