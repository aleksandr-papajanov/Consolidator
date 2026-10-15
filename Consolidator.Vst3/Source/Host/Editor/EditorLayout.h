#pragma once

#include <array>

#include <JuceHeader.h>

namespace consolidator::vst3::host::editor
{

inline constexpr int kSidebarWidth = 112;
inline constexpr int kTrackListWidth = 576;
inline constexpr int kProcessingWidth = 1100;
inline constexpr int kEditorHeight = 904;

struct EditorBounds
{
    std::array<juce::Rectangle<int>, 5> sidebarActions;
    juce::Rectangle<int> trackTitle;
    juce::Rectangle<int> trackButtons;
    juce::Rectangle<int> bankManager;
    juce::Rectangle<int> equalizer;
    juce::Rectangle<int> channelStrip;
    juce::Rectangle<int> status;
};

class EditorLayout final
{
public:
    [[nodiscard]] static int Width(bool trackListExpanded);
    [[nodiscard]] static EditorBounds Calculate(bool trackListExpanded, int titleTextWidth);
};

} // namespace consolidator::vst3::host::editor
