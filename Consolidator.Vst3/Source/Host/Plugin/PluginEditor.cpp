#include "Host/Plugin/PluginEditor.h"

#include <string>
#include <array>
#include <utility>
#include <vector>

#include "Host/Plugin/WindowsPluginHost.h"
#include "Host/Editor/EditorLayout.h"
#include "Shared/Theme/UiColors.h"

namespace consolidator::vst3::host::plugin
{

namespace
{
constexpr std::array<int, 3> kScalePercentages{ 75, 100, 125 };
}

PluginEditor::PluginEditor(PluginProcessor& processor)
    : AudioProcessorEditor(processor), processor_(processor),
      presentation_({
          processor.InstanceId(), processor.IsReady(),
          [this](const std::string& selector, std::vector<shared::protocol::ProtocolValue> body)
              { return processor_.Request(selector, std::move(body)); },
          [this] { return processor_.PresentationSampleRate(); },
          [this]
          {
              const auto* peer = getPeer();
              return peer != nullptr && IsEditorWindowFocused(peer->getNativeHandle());
          },
          [](const std::string& message) { WriteManagedLog(nullptr, message.c_str()); },
          [this] { return CycleScale(); },
          [this] { ApplyScale(); } })
{
    addAndMakeVisible(presentation_);
    setResizable(false, false);
    ApplyScale();
    processor_.SetOutputListener([this](const shared::protocol::ProtocolFrame& frame)
    {
        presentation_.HandleOutput(frame);
    });
    presentation_.Start();
}

PluginEditor::~PluginEditor()
{
    // Stop delivery before presentation closes gestures and removes client callbacks.
    processor_.SetOutputListener({});
}

int PluginEditor::CycleScale()
{
    scaleIndex_ = (scaleIndex_ + 1) % kScalePercentages.size();
    ApplyScale();
    return kScalePercentages[scaleIndex_];
}

void PluginEditor::ApplyScale()
{
    const auto percentage = kScalePercentages[scaleIndex_];
    presentation_.setSize(presentation_.LogicalWidth(), editor::kEditorHeight);
    presentation_.setTransform(juce::AffineTransform::scale(static_cast<float>(percentage) / 100.0F));
    const int width = presentation_.LogicalWidth() * percentage / 100;
    const int height = editor::kEditorHeight * percentage / 100;
    setResizeLimits(width, height, width, height);
    setSize(width, height);
}

void PluginEditor::paint(juce::Graphics& graphics)
{
    graphics.fillAll(shared::ui::kBackground);
}

} // namespace consolidator::vst3::host::plugin
