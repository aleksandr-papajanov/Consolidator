#pragma once

#include <JuceHeader.h>

#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::equalizer::presentation
{

inline void DrawMarkerListRow(juce::Graphics& graphics, int width, int height,
    const juce::String& label, bool active, bool enabled)
{
    graphics.setColour(enabled ? (active ? shared::ui::kActive : shared::ui::kText) : shared::ui::kDisabled);
    const juce::Rectangle<float> marker{ 6.0F, static_cast<float>(height) * 0.5F - 4.0F, 8.0F, 8.0F };
    if (active)
    {
        graphics.fillRect(marker);
    }
    else
    {
        graphics.drawRect(marker, 1.0F);
    }
    graphics.setFont(shared::ui::ControlFont());
    graphics.drawText(label, 20, 0, width - 24, height, juce::Justification::centredLeft);
}

} // namespace consolidator::vst3::features::equalizer::presentation
