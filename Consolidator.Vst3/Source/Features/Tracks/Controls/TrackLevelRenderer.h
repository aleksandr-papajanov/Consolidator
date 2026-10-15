#pragma once

#include <JuceHeader.h>

namespace consolidator::vst3::features::tracks::controls
{

class TrackLevelRenderer final
{
public:
    // Draws the level dot and returns the remaining track-label bounds.
    [[nodiscard]] static juce::Rectangle<int> Draw(juce::Graphics& graphics,
        juce::Rectangle<int> bounds, double alpha);
};

} // namespace consolidator::vst3::features::tracks::controls
