#pragma once

#include <functional>

#include <JuceHeader.h>

namespace consolidator::vst3::shared::controls
{

class MarkerControl final : public juce::Component, public juce::SettableTooltipClient
{
public:
    MarkerControl(juce::String label, juce::Colour color);
    void SetProcessorState(bool active, bool bypassed);
    [[nodiscard]] int PreferredWidth() const;
    static void DrawSquare(juce::Graphics& graphics, juce::Rectangle<float> bounds,
        juce::Colour color, bool checked, bool bypassed = false);
    static void DrawRow(juce::Graphics& graphics, juce::Rectangle<int> bounds, const juce::String& label,
        juce::Colour color, bool checked, int markerSize, float fontHeight, bool bypassed = false);
    void paint(juce::Graphics& graphics) override;
    void mouseDown(const juce::MouseEvent& event) override;

    std::function<void()> onToggle;
    std::function<void()> onReset;

private:
    [[nodiscard]] static juce::Colour StateColor(juce::Colour color, bool bypassed);

    juce::String label_;
    juce::Colour color_;
    bool checked_{};
    bool bypassed_{};
};

} // namespace consolidator::vst3::shared::controls
