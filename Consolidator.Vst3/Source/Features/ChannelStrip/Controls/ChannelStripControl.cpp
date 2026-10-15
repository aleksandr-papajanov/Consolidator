#include "Features/ChannelStrip/Controls/ChannelStripControl.h"

#include <cstddef>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "Features/ChannelStrip/Presentation/ProcessorCatalog.h"
#include "Shared/Presentation/ProcessorPresentation.h"
#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::channelstrip::controls
{

void ChannelStripControl::Clear()
{
    controls_.clear();
    headers_.clear();
}

void ChannelStripControl::Rebuild(bindings::ChannelStripBindings& bindings)
{
    Clear();
    auto& parameters = bindings.Parameters();
    std::vector<std::unique_ptr<shared::controls::parameter::ParameterControl>> controls;
    controls.reserve(parameters.Models().size());
    for (const auto& model : parameters.Models())
    {
        const auto& path = model->State().Path();
        const auto& section = presentation::SectionFor(path.substr(0, path.find('.')));
        const auto& slot = presentation::SlotFor(section, path);
        if (slot.kind == presentation::ParameterSlotKind::Heading)
        {
            const auto context = path.substr(0, path.find('.'));
            const auto& processor = shared::presentation::ProcessorFor(context);
            auto marker = std::make_unique<shared::controls::MarkerControl>(
                juce::String::fromUTF8(processor.label.data(), static_cast<int>(processor.label.size())), processor.color);
            const auto intents = parameters.Intents(path);
            marker->onToggle = [model = model.get(), commit = intents.commit]
            {
                commit(model->State().Path(), model->ButtonValue() ? 0 : 1);
            };
            marker->onReset = [context, reset = intents.reset] { reset(context); };
            addAndMakeVisible(*marker);
            headers_.push_back({ model.get(), std::move(marker) });
            continue;
        }
        const auto label = slot.label.empty() ? juce::String(model->State().Label())
            : juce::String::fromUTF8(slot.label.data(), static_cast<int>(slot.label.size()));
        controls.push_back(std::make_unique<shared::controls::parameter::ParameterControl>(
            *model, parameters.Intents(path), label, 0));
    }
    controls_ = std::move(controls);
    for (const auto& control : controls_)
    {
        addAndMakeVisible(*control);
    }
    resized();
}

void ChannelStripControl::paint(juce::Graphics& graphics)
{
    for (std::size_t index = 0; index < presentation::kSections.size(); ++index)
    {
        const int left = presentation::kSections[index].left;
        if (index > 0)
        {
            graphics.setColour(shared::ui::kLines);
            graphics.drawVerticalLine(left, 8.0F, static_cast<float>(getHeight() - 12));
        }
    }
    graphics.setColour(shared::ui::kActiveText);
    graphics.setFont(shared::ui::ControlFont());
    graphics.drawText("INPUT", presentation::SectionFor("gain_staging").left + 19, 52, 112, 20, juce::Justification::centred);
    graphics.drawText("OUTPUT", presentation::SectionFor("gain_staging").left + 169, 52, 112, 20, juce::Justification::centred);
}

void ChannelStripControl::ApplyProcessorStatus(const shared::clients::RegistryProcessor& processor)
{
    for (const auto& header : headers_)
    {
        if (header.model->State().Path() == processor.id + ".bypass")
        {
            header.marker->SetProcessorState(processor.active, processor.bypass);
        }
    }
}

void ChannelStripControl::RefreshPath(const std::string& path)
{
    for (const auto& control : controls_)
    {
        if (control->Path() == path)
        {
            control->Refresh();
        }
    }
}

void ChannelStripControl::SetScopeColor(juce::Colour color)
{
    for (const auto& control : controls_)
    {
        control->SetScopeColor(color);
    }
}

void ChannelStripControl::SetEditing(bool enabled, bool navigation)
{
    setEnabled(enabled);
    for (const auto& header : headers_)
    {
        header.marker->setEnabled(navigation);
    }
    for (const auto& control : controls_)
    {
        control->setEnabled(enabled && control->Model().State().Enabled());
    }
}

void ChannelStripControl::FinishPreview(bool accepted)
{
    for (const auto& control : controls_)
    {
        control->FinishPreview(accepted);
    }
}

void ChannelStripControl::resized()
{
    for (const auto& header : headers_)
    {
        const auto& path = header.model->State().Path();
        const auto& section = presentation::SectionFor(path.substr(0, path.find('.')));
        auto& marker = *header.marker;
        const int markerWidth = marker.PreferredWidth();
        marker.setBounds(section.left + (section.width - markerWidth) / 2, 8,
            markerWidth, shared::ui::kButtonHeight);
    }
    for (const auto& control : controls_)
    {
        const auto& path = control->Path();
        const auto& section = presentation::SectionFor(path.substr(0, path.find('.')));
        const auto& slot = presentation::SlotFor(section, path);
        const int center = section.left + slot.center;
        control->setBounds(slot.kind == presentation::ParameterSlotKind::Toolbar
            ? juce::Rectangle<int>{ center - 48, 76, 96, shared::ui::kButtonHeight }
            : juce::Rectangle<int>{ center - 56, 116 + slot.row * 100, 112, 92 });
    }
}

} // namespace consolidator::vst3::features::channelstrip::controls
