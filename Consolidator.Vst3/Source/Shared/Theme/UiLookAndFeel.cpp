#include "Shared/Theme/UiLookAndFeel.h"

#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::shared::ui
{

namespace
{
bool Flashing(const juce::Button& button)
{
    return static_cast<bool>(button.getProperties().getWithDefault(kActionFlashProperty, false));
}

bool ValueDisplay(const juce::Button& button)
{
    return static_cast<bool>(button.getProperties().getWithDefault(kAcceptedValueProperty, false));
}

bool Active(const juce::Button& button, bool down)
{
    return button.getToggleState() || Flashing(button) || (!ValueDisplay(button) && down);
}
}

UiLookAndFeel::UiLookAndFeel()
{
    setColour(juce::TextButton::buttonColourId, kBackground);
    setColour(juce::TextButton::buttonOnColourId, kActive);
    setColour(juce::TextButton::textColourOffId, kActiveText);
    setColour(juce::TextButton::textColourOnId, kBackground);
    setColour(juce::ComboBox::backgroundColourId, kBackground);
    setColour(juce::ComboBox::textColourId, kActiveText);
    setColour(juce::ComboBox::outlineColourId, kLines);
    setColour(juce::ComboBox::arrowColourId, kActiveText);
    setColour(juce::PopupMenu::backgroundColourId, kBackground);
    setColour(juce::PopupMenu::textColourId, kActiveText);
    setColour(juce::PopupMenu::highlightedBackgroundColourId, kActive);
    setColour(juce::PopupMenu::highlightedTextColourId, kBackground);
}

juce::Font UiLookAndFeel::getTextButtonFont(juce::TextButton&, int) { return ControlFont(); }
juce::Font UiLookAndFeel::getComboBoxFont(juce::ComboBox&) { return ControlFont(); }
juce::Font UiLookAndFeel::getPopupMenuFont() { return ControlFont(); }

void UiLookAndFeel::drawButtonBackground(juce::Graphics& graphics, juce::Button& button,
    const juce::Colour&, bool highlighted, bool down)
{
    const bool active = Active(button, down);
    auto color = button.findColour(active ? juce::TextButton::buttonOnColourId : juce::TextButton::buttonColourId);
    if (highlighted && button.isEnabled() && !ValueDisplay(button))
    {
        color = color.brighter(0.08F);
    }
    const auto bounds = button.getLocalBounds();
    graphics.setColour(color);
    graphics.fillRect(bounds);
    graphics.setColour(kLines);
    // Joined buttons share one border, owned by the button below or to the right.
    graphics.fillRect(0, 0, bounds.getWidth(), 1);
    graphics.fillRect(0, 0, 1, bounds.getHeight());
    if (!button.isConnectedOnRight())
    {
        graphics.fillRect(bounds.getWidth() - 1, 0, 1, bounds.getHeight());
    }
    if (!button.isConnectedOnBottom())
    {
        graphics.fillRect(0, bounds.getHeight() - 1, bounds.getWidth(), 1);
    }
}

void UiLookAndFeel::drawButtonText(juce::Graphics& graphics, juce::TextButton& button, bool, bool down)
{
    const bool active = Active(button, down);
    const auto color = button.findColour(active ? juce::TextButton::textColourOnId : juce::TextButton::textColourOffId);
    // A temporary input lock does not dim accepted parameter or row values.
    graphics.setColour(button.isEnabled() || ValueDisplay(button) || Flashing(button) ? color : kDisabled);
    graphics.setFont(ControlFont());
    const int padding = button.getWidth() < kButtonHeight ? 2 : 6;
    graphics.drawText(button.getButtonText(), button.getLocalBounds().reduced(padding, 0), juce::Justification::centred);
    if (button.getProperties().contains(kGroupMarkerColorProperty))
    {
        const auto argb = static_cast<juce::uint32>(
            static_cast<juce::int64>(button.getProperties()[kGroupMarkerColorProperty]));
        graphics.setColour(juce::Colour(argb));
        graphics.fillEllipse(static_cast<float>(button.getWidth() - 8), 4, 4, 4);
    }
}

void UiLookAndFeel::drawComboBox(juce::Graphics& graphics, int width, int height, bool,
    int, int, int, int, juce::ComboBox& box)
{
    const juce::Rectangle<int> bounds{ 0, 0, width, height };
    graphics.setColour(box.findColour(juce::ComboBox::backgroundColourId));
    graphics.fillRect(bounds);
    graphics.setColour(box.findColour(juce::ComboBox::outlineColourId));
    graphics.drawRect(bounds, 1);
    const float x = static_cast<float>(width - 14);
    const float y = static_cast<float>(height) * 0.5F;
    juce::Path arrow;
    arrow.startNewSubPath(x - 4, y - 2);
    arrow.lineTo(x, y + 2);
    arrow.lineTo(x + 4, y - 2);
    graphics.setColour(box.isEnabled() ? box.findColour(juce::ComboBox::arrowColourId) : kDisabled);
    graphics.strokePath(arrow, juce::PathStrokeType(1.5F));
}

void UiLookAndFeel::positionComboBoxText(juce::ComboBox& box, juce::Label& label)
{
    label.setBounds(6, 0, box.getWidth() - 30, box.getHeight());
    label.setFont(ControlFont());
}

} // namespace consolidator::vst3::shared::ui
