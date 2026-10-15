#pragma once

#include <JuceHeader.h>

namespace consolidator::vst3::shared::controls::dial
{

struct DialGeometry
{
    juce::Point<float> center;
    float radius;
    juce::Rectangle<int> label;

    [[nodiscard]] static DialGeometry FromBounds(juce::Rectangle<int> bounds);
    [[nodiscard]] static float Angle(double position);
    [[nodiscard]] juce::Point<float> Point(double position) const;
};

} // namespace consolidator::vst3::shared::controls::dial
