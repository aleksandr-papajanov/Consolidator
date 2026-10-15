#pragma once

#include <array>

#include <JuceHeader.h>

namespace consolidator::vst3::host::editor
{

inline constexpr int kSidebarWidth = 112;
inline constexpr int kTrackListWidth = 576;
inline constexpr int kProcessingWidth = 1100;
inline constexpr int kEditorHeight = 1104;

struct EditorBounds
{
    std::array<juce::Rectangle<int>, 5> sidebarActions;
    std::array<juce::Rectangle<int>, 4> trackActions;
    juce::Rectangle<int> brandingTitle;
    juce::Rectangle<int> trackTitle;
    juce::Rectangle<int> trackButtons;
    juce::Rectangle<int> trackList;
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
