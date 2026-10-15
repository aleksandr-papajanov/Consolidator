#pragma once

#include <cstddef>

#include <JuceHeader.h>

#include "Host/Plugin/PluginProcessor.h"
#include "Host/Editor/EditorPresentation.h"

namespace consolidator::vst3::host::plugin
{

class PluginEditor final : public juce::AudioProcessorEditor
{
public:
    explicit PluginEditor(PluginProcessor& processor);
    ~PluginEditor() override;
    void paint(juce::Graphics& graphics) override;

private:
    [[nodiscard]] int CycleScale();
    void ApplyScale();
    PluginProcessor& processor_;
    std::size_t scaleIndex_{ 1 };
    editor::EditorPresentation presentation_;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(PluginEditor)
};

} // namespace consolidator::vst3::host::plugin
