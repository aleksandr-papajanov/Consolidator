#pragma once

#include <array>
#include <functional>

#include <JuceHeader.h>

namespace consolidator::vst3::features::equalizer::controls
{

class AnalyzerOverlayList final : public juce::Component, private juce::ListBoxModel
{
public:
    explicit AnalyzerOverlayList(std::function<void(bool, bool, bool)> changed);
    void resized() override;
    void enablementChanged() override;

private:
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& graphics, int width, int height, bool selected) override;
    void listBoxItemClicked(int row, const juce::MouseEvent& event) override;
    std::function<void(bool, bool, bool)> changed_;
    std::array<bool, 3> visible_{ true, true, true };
    juce::ListBox list_{ "Analyzer overlays", this };
};

} // namespace consolidator::vst3::features::equalizer::controls
