#pragma once

#include <functional>
#include <string>

#include <JuceHeader.h>

namespace consolidator::vst3::features::equalizer::controls
{

class AnalyzerSourceList final : public juce::Component, private juce::ListBoxModel
{
public:
    explicit AnalyzerSourceList(std::function<void()> changed);
    [[nodiscard]] const std::string& ContextPath() const;
    void resized() override;
    void enablementChanged() override;

private:
    int getNumRows() override;
    void paintListBoxItem(int row, juce::Graphics& graphics, int width, int height, bool selected) override;
    void selectedRowsChanged(int row) override;
    juce::String getTooltipForRow(int row) override;
    std::function<void()> changed_;
    int selected_{};
    juce::ListBox list_{ "Analyzer sources", this };
};

} // namespace consolidator::vst3::features::equalizer::controls
