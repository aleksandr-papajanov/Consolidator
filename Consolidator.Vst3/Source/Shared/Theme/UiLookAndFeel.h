#pragma once

#include <JuceHeader.h>

namespace consolidator::vst3::shared::ui
{

class UiLookAndFeel final : public juce::LookAndFeel_V4
{
public:
    UiLookAndFeel();
    juce::Font getTextButtonFont(juce::TextButton&, int) override;
    void drawButtonBackground(juce::Graphics&, juce::Button&, const juce::Colour&, bool, bool) override;
    void drawButtonText(juce::Graphics&, juce::TextButton&, bool, bool) override;
    juce::Font getComboBoxFont(juce::ComboBox&) override;
    void drawComboBox(juce::Graphics&, int, int, bool, int, int, int, int, juce::ComboBox&) override;
    void positionComboBoxText(juce::ComboBox&, juce::Label&) override;
    juce::Font getPopupMenuFont() override;
};

} // namespace consolidator::vst3::shared::ui
