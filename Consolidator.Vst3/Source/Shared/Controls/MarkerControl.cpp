#include "Shared/Controls/MarkerControl.h"

#include <utility>

#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::shared::controls
{

MarkerControl::MarkerControl(juce::String label, juce::Colour color)
    : label_(std::move(label)), color_(color)
{
    setMouseCursor(juce::MouseCursor::PointingHandCursor);
    setTooltip(label_ + ": click to bypass, double-click to reset");
}

void MarkerControl::SetProcessorState(bool active, bool bypassed)
{
    checked_ = active;
    bypassed_ = bypassed;
    repaint();
}

int MarkerControl::PreferredWidth() const
{
    const juce::Font font(juce::FontOptions("Arial", ui::kSectionFontHeight, juce::Font::plain));
    return 32 + juce::GlyphArrangement::getStringWidthInt(font, label_);
}

void MarkerControl::DrawSquare(juce::Graphics& graphics, juce::Rectangle<float> bounds,
    juce::Colour color, bool checked, bool bypassed)
{
    graphics.setColour(StateColor(color, bypassed));
    if (checked)
    {
        graphics.fillRect(bounds);
    }
    else
    {
        graphics.drawRect(bounds, 1.0F);
    }
}

void MarkerControl::DrawRow(juce::Graphics& graphics, juce::Rectangle<int> bounds, const juce::String& label,
    juce::Colour color, bool checked, int markerSize, float fontHeight, bool bypassed)
{
    const juce::Rectangle<float> marker{
        static_cast<float>(bounds.getX() + 6),
        static_cast<float>(bounds.getCentreY()) - static_cast<float>(markerSize) * 0.5F,
        static_cast<float>(markerSize), static_cast<float>(markerSize) };
    DrawSquare(graphics, marker, color, checked, bypassed);
    graphics.setFont(juce::Font(juce::FontOptions("Arial", fontHeight, juce::Font::plain)));
    graphics.setColour(StateColor(color, bypassed));
    graphics.drawText(label, bounds.withTrimmedLeft(markerSize + 12).withTrimmedRight(4),
        juce::Justification::centredLeft);
}

void MarkerControl::paint(juce::Graphics& graphics)
{
    DrawRow(graphics, getLocalBounds(), label_, color_, checked_, 14, ui::kSectionFontHeight, bypassed_);
}

juce::Colour MarkerControl::StateColor(juce::Colour color, bool bypassed)
{
    return bypassed ? ui::kDisabled : color;
}

void MarkerControl::mouseDown(const juce::MouseEvent& event)
{
    if (!isEnabled() || !event.mods.isLeftButtonDown())
    {
        return;
    }
    if (event.getNumberOfClicks() == 2)
    {
        if (onReset)
        {
            onReset();
        }
    }
    else if (event.getNumberOfClicks() == 1 && onToggle)
    {
        onToggle();
    }
}

} // namespace consolidator::vst3::shared::controls
