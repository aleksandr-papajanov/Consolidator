#include "Features/Equalizer/Controls/AnalyzerRenderer.h"

#include <cmath>

#include "Features/Equalizer/Presentation/AnalyzerScale.h"
#include "Shared/Theme/UiColors.h"

namespace consolidator::vst3::features::equalizer::controls
{

juce::Point<float> AnalyzerRenderer::HandlePosition(const features::equalizer::presentation::AnalyzerFilter& filter,
    const juce::Rectangle<float>& plot)
{
    const double x = filter.type == features::equalizer::presentation::AnalyzerFilterType::Gain
        ? 0 : presentation::AnalyzerScale::FrequencyPosition(filter.frequency);
    return { plot.getX() + plot.getWidth() * static_cast<float>(x),
        plot.getY() + plot.getHeight() * static_cast<float>(presentation::AnalyzerScale::GainPosition(filter.gain)) };
}

void AnalyzerRenderer::BuildPath(juce::Path& path, std::span<const double> values) const
{
    path.clear();
    if (values.size() < 2)
    {
        return;
    }
    path.preallocateSpace(static_cast<int>(values.size() * 3 + 1));
    bool drawing = false;
    for (std::size_t point = 0; point < values.size(); ++point)
    {
        const double value = values[point];
        if (!std::isfinite(value))
        {
            drawing = false;
            continue;
        }
        if (point > 0 && point + 1 < values.size()
            && value == values[point - 1] && value == values[point + 1])
        {
            continue;
        }
        const float x = plot_.getX() + plot_.getWidth() * static_cast<float>(point) / static_cast<float>(values.size() - 1);
        const float y = plot_.getY() + plot_.getHeight() * static_cast<float>(value);
        if (drawing)
        {
            path.lineTo(x, y);
        }
        else
        {
            path.startNewSubPath(x, y);
            drawing = true;
        }
    }
}

void AnalyzerRenderer::SetPlot(juce::Rectangle<float> plot)
{
    if (plot_ == plot)
    {
        return;
    }
    plot_ = plot;
    curvesRevision_.reset();
    gridLines_.clear();
    plotClip_.clear();
    plotClip_.addRectangle(plot);
    gainHandle_.clear();
    gainHandle_.startNewSubPath(-6, 0);
    gainHandle_.lineTo(3, -5);
    gainHandle_.lineTo(3, 5);
    gainHandle_.closeSubPath();
    for (const double frequency : { 100.0, 200.0, 500.0, 1000.0, 2000.0, 5000.0, 10000.0 })
    {
        const float x = std::round(plot.getX() + plot.getWidth() * static_cast<float>(presentation::AnalyzerScale::FrequencyPosition(frequency))) + 0.5F;
        gridLines_.startNewSubPath(x, plot.getY());
        gridLines_.lineTo(x, plot.getBottom());
    }
}

void AnalyzerRenderer::UpdateCurves(const features::equalizer::presentation::AnalyzerCurves& curves,
    const std::optional<features::equalizer::presentation::AnalyzerCurves::Curve>& allBanks,
    std::uint64_t revision)
{
    if (curvesRevision_ == revision)
    {
        return;
    }
    curvesRevision_ = revision;
    filterPaths_.resize(curves.filters.size());
    for (std::size_t index = 0; index < curves.filters.size(); ++index)
    {
        BuildPath(filterPaths_[index], curves.filters[index]);
    }
    BuildPath(combinedPath_, curves.combined);
    if (allBanks)
    {
        BuildPath(allBanksPath_, *allBanks);
    }
    else
    {
        allBanksPath_.clear();
    }
}

void AnalyzerRenderer::UpdateSpectrum(std::span<const double> source, std::span<const double> group)
{
    BuildPath(sourcePath_, source);
    BuildPath(groupPath_, group);
}

void AnalyzerRenderer::UpdateReference(std::span<const double> reference)
{
    BuildPath(referencePath_, reference);
}

void AnalyzerRenderer::Draw(juce::Graphics& graphics, const AnalyzerRenderState& state) const
{
    graphics.fillAll(shared::ui::kAnalyzerBackground);
    if (!state.ready)
    {
        return;
    }
    graphics.setColour(shared::ui::kLines);
    graphics.strokePath(gridLines_, juce::PathStrokeType(1));
    {
        const juce::Graphics::ScopedSaveState saved(graphics);
        graphics.reduceClipRegion(plotClip_);
        if (state.hasSource)
        {
            graphics.setColour(shared::ui::kAnalyzerSpectrum);
            graphics.strokePath(sourcePath_, juce::PathStrokeType(1));
        }
        if (state.hasGroup)
        {
            graphics.setColour(state.groupColor);
            graphics.strokePath(groupPath_, juce::PathStrokeType(1));
        }
        if (state.showReference)
        {
            graphics.setColour(shared::ui::kGainStaging);
            graphics.strokePath(referencePath_, juce::PathStrokeType(1));
        }
        for (std::size_t index = 0; index < state.filters.size(); ++index)
        {
            graphics.setColour(state.filters[index].active
                ? shared::ui::kAnalyzerFilters[index] : shared::ui::kDisabled);
            graphics.strokePath(filterPaths_[index], juce::PathStrokeType(1.2F));
        }
        if (state.showAllBanks)
        {
            graphics.setColour(shared::ui::kAnalyzerAllBanks);
            graphics.strokePath(allBanksPath_, juce::PathStrokeType(1.2F));
        }
        graphics.setColour(state.bankActive ? shared::ui::kActive : shared::ui::kAnalyzerCombinedInactive);
        graphics.strokePath(combinedPath_, juce::PathStrokeType(1.2F));
    }
    for (std::size_t index = 0; index < state.filters.size(); ++index)
    {
        const bool active = state.filters[index].active;
        const auto position = HandlePosition(state.filters[index], state.plot);
        graphics.setColour(!active ? shared::ui::kDisabled : state.groupScope ? state.groupColor :
            static_cast<int>(index) == state.selected ? shared::ui::kAnalyzerSelectedHandle : shared::ui::kAnalyzerHandle);
        if (state.filters[index].type == features::equalizer::presentation::AnalyzerFilterType::Gain)
        {
            const auto transform = juce::AffineTransform::translation(position.x, position.y);
            if (active)
            {
                graphics.fillPath(gainHandle_, transform);
            }
            else
            {
                graphics.strokePath(gainHandle_, juce::PathStrokeType(1.2F), transform);
            }
        }
        else
        {
            if (active)
            {
                graphics.fillEllipse(position.x - 4, position.y - 4, 8, 8);
            }
            else
            {
                graphics.drawEllipse(position.x - 4, position.y - 4, 8, 8, 1.2F);
            }
        }
    }
}

} // namespace consolidator::vst3::features::equalizer::controls
