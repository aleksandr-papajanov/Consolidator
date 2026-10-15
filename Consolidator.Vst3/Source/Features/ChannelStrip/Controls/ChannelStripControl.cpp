#include "Features/ChannelStrip/Controls/ChannelStripControl.h"

#include <algorithm>
#include <utility>
#include <stdexcept>

#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"
#include "Features/ChannelStrip/Presentation/ProcessorCatalog.h"

namespace consolidator::vst3::features::channelstrip::controls
{

void ChannelStripControl::Clear()
{
    controls_.clear();
    positions_.clear();
}

void ChannelStripControl::Rebuild(bindings::ChannelStripBindings& bindings)
{
    auto& parameters = bindings.Parameters();
    std::vector<std::unique_ptr<shared::controls::parameter::ParameterControl>> controls;
    controls.reserve(parameters.Models().size());
    for (const auto& model : parameters.Models())
    {
        controls.push_back(std::make_unique<shared::controls::parameter::ParameterControl>(
            *model, parameters.Intents(model->State().Path())));
    }
    Replace(std::move(controls), presentation::kSections);
}

void ChannelStripControl::Replace(std::vector<std::unique_ptr<shared::controls::parameter::ParameterControl>> controls,
    std::span<const shared::controls::parameter::presentation::ParameterPresentation> presentations)
{
    controls_ = std::move(controls);
    positions_.clear();
    for (const auto& control : controls_)
    {
        bool placed = false;
        for (std::size_t column = 0; column < presentations.size(); ++column)
        {
            const auto paths = presentations[column].orderedPaths;
            const auto path = std::find(paths.begin(), paths.end(), control->Path());
            if (path != paths.end())
            {
                positions_.emplace_back(static_cast<int>(column), static_cast<int>(path - paths.begin()));
                placed = true;
                break;
            }
        }
        if (!placed)
        {
            throw std::invalid_argument("Parameter has no channel strip slot: " + control->Path());
        }
        addAndMakeVisible(*control);
    }
    resized();
}

void ChannelStripControl::paint(juce::Graphics& graphics)
{
    graphics.setColour(shared::ui::kActiveText);
    graphics.setFont(juce::Font(juce::FontOptions("Arial", shared::ui::kSectionFontHeight, juce::Font::plain)));
    for (std::size_t column = 0; column < presentation::kSections.size(); ++column)
    {
        const auto title = presentation::kSections[column].context;
        graphics.drawText(juce::String::fromUTF8(title.data(), static_cast<int>(title.size())).toUpperCase(),
            static_cast<int>(column) * 220,
            4, 220, 28, juce::Justification::centred);
        graphics.setColour(shared::ui::kLines);
        if (column > 0)
        {
            graphics.drawVerticalLine(static_cast<int>(column) * 220, 4.0F, static_cast<float>(getHeight() - 8));
        }
        graphics.setColour(shared::ui::kActiveText);
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

void ChannelStripControl::SetEditing(bool enabled)
{
    setEnabled(enabled);
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
    for (std::size_t index = 0; index < controls_.size(); ++index)
    {
        const auto [column, row] = positions_.at(index);
        auto& control = *controls_[index];
        juce::Rectangle<int> bounds{ column * 220 + 58, 40 + row * 88, 104, 84 };
        if (control.Model().Dial() == nullptr && !control.Model().HasChoices())
        {
            bounds = bounds.withSizeKeepingCentre(bounds.getWidth() - 16, shared::ui::kButtonHeight);
        }
        control.setBounds(bounds);
    }
}

} // namespace consolidator::vst3::features::channelstrip::controls
