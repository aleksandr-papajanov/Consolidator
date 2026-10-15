#pragma once

#include <optional>
#include <utility>

#include <JuceHeader.h>

#include "Shared/Editing/ParameterIntents.h"
#include "Shared/Controls/Dial/DialViewModel.h"

namespace consolidator::vst3::shared::controls::dial
{

// Port of DialControlCore/DialRenderer/DialValueEditor; message-thread only.
class DialControl final : public juce::Component, private juce::Timer
{
public:
    DialControl(shared::controls::dial::DialViewModel& model,
        shared::editing::ParameterIntents intents);
    ~DialControl() override;
    void Refresh();
    void SetScopeColor(juce::Colour color);
    void FinishPreview(bool accepted);
    void paint(juce::Graphics& graphics) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    void DrawArc(juce::Graphics& graphics, juce::Point<float> center, float radius,
        double start, double end, juce::Colour color, float thickness) const;
    void timerCallback() override;

    shared::controls::dial::DialViewModel& model_;
    shared::editing::ParameterIntents intents_;
    juce::Colour scopeColor_;
    double gesturePosition_{};
    float lastY_{};
    bool dragging_{};
    bool showValue_{};
};

} // namespace consolidator::vst3::shared::controls::dial
