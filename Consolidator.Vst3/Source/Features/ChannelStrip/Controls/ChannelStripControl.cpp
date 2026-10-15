#include "Features/ChannelStrip/Controls/ChannelStripControl.h"

#include <algorithm>
#include <utility>
#include <stdexcept>

#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"
#include "Features/ChannelStrip/Presentation/ProcessorCatalog.h"

namespace consolidator::vst3::features::channelstrip::controls
{

ChannelStripControl::ChannelStripControl()
{
    gainStagingReset_.setTooltip("Reset Gain Staging");
    gainStagingReset_.setConnectedEdges(juce::Button::ConnectedOnLeft);
    addAndMakeVisible(gainStagingReset_);
}

void ChannelStripControl::Clear()
{
    controls_.clear();
    positions_.clear();
    gainStagingReset_.onClick = nullptr;
    gainStagingReset_.setEnabled(false);
}

void ChannelStripControl::Rebuild(bindings::ChannelStripBindings& bindings)
{
    auto& parameters = bindings.Parameters();
    std::vector<std::unique_ptr<shared::controls::parameter::ParameterControl>> controls;
    controls.reserve(parameters.Models().size());
    for (const auto& model : parameters.Models())
    {
        const auto& path = model->State().Path();
        const auto label = path == "gain_staging.bypass" ? juce::String("B") :
            (path == "gain_staging.input_leveler" || path == "gain_staging.output_leveler")
                ? juce::String("Level") : juce::String(model->State().Label());
        controls.push_back(std::make_unique<shared::controls::parameter::ParameterControl>(
            *model, parameters.Intents(path), label,
            path == "gain_staging.bypass" ? juce::Button::ConnectedOnRight : 0));
    }
    gainStagingReset_.onClick = [reset = parameters.Intents("gain_staging").reset] { reset("gain_staging"); };
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
        const int left = column == 0 ? 0 : (static_cast<int>(column) + 1) * 220;
        const int width = column == 0 ? 440 : 220;
        auto label = juce::String::fromUTF8(title.data(), static_cast<int>(title.size())).toUpperCase();
        label = label.replaceCharacter('_', ' ');
        graphics.drawText(label, left, 4, width, 28, juce::Justification::centred);
        graphics.setColour(shared::ui::kLines);
        if (column > 0)
        {
            graphics.drawVerticalLine(left, 4.0F, static_cast<float>(getHeight() - 8));
        }
        graphics.setColour(shared::ui::kActiveText);
    }
    graphics.setFont(shared::ui::ControlFont());
    graphics.drawText("INPUT", 0, 70, 220, 24, juce::Justification::centred);
    graphics.drawText("OUTPUT", 220, 70, 220, 24, juce::Justification::centred);
    graphics.drawText("UTILITY", 0, 412, 440, 28, juce::Justification::centred);
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
    gainStagingReset_.setEnabled(navigation && static_cast<bool>(gainStagingReset_.onClick));
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
    gainStagingReset_.setBounds(220, 36, 28, shared::ui::kButtonHeight);
    for (std::size_t index = 0; index < controls_.size(); ++index)
    {
        const auto [column, row] = positions_.at(index);
        auto& control = *controls_[index];
        juce::Rectangle<int> bounds{ column * 220 + 58, 40 + row * 88, 104, 84 };
        if (control.Model().Dial() == nullptr && !control.Model().HasChoices())
        {
            bounds = bounds.withSizeKeepingCentre(bounds.getWidth() - 16, shared::ui::kButtonHeight);
        }
        if (column == 0)
        {
            if (row == 8)
            {
                bounds = { 192, 36, 28, shared::ui::kButtonHeight };
            }
            else if (row == 7)
            {
                bounds = { 168, 448, 104, 84 };
            }
            else
            {
                const bool output = row >= 3;
                const int slot = output ? row - 3 : row;
                const int left = (output ? 220 : 0) + 58;
                bounds = slot == 0 ? juce::Rectangle<int>{ left + 8, 98, 88, shared::ui::kButtonHeight }
                    : juce::Rectangle<int>{ left, 138 + (slot - 1) * 88, 104, 84 };
            }
        }
        else
        {
            bounds.translate(220, 0);
        }
        control.setBounds(bounds);
    }
}

} // namespace consolidator::vst3::features::channelstrip::controls
