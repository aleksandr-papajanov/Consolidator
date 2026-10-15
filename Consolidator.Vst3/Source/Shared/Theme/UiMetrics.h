#pragma once

#include <JuceHeader.h>

namespace consolidator::vst3::shared::ui
{

inline constexpr int kButtonHeight = 28;
inline constexpr int kControlGap = 4;
inline constexpr int kBankCellSize = 24;
inline constexpr float kControlFontHeight = 12.0F;
inline constexpr float kSectionFontHeight = 16.0F;
inline constexpr float kTitleFontHeight = 24.0F;
inline const juce::Identifier kActionFlashProperty{ "consolidatorActionFlash" };
inline const juce::Identifier kAcceptedValueProperty{ "consolidatorAcceptedValue" };
inline const juce::Identifier kGroupMarkerColorProperty{ "consolidatorGroupMarkerColor" };

inline juce::Font ControlFont()
{
    return juce::Font(juce::FontOptions("Arial", kControlFontHeight, juce::Font::plain));
}

} // namespace consolidator::vst3::shared::ui
