#include "Features/Equalizer/Controls/AnalyzerControl.h"

#include <algorithm>
#include <cmath>
#include <utility>

#include "Features/Equalizer/Presentation/AnalyzerCurveCalculator.h"

#include "Features/Equalizer/Controls/AnalyzerRenderer.h"

namespace consolidator::vst3::features::equalizer::controls
{

AnalyzerControl::AnalyzerControl(features::equalizer::viewmodels::AnalyzerViewModel& model, features::equalizer::bindings::AnalyzerIntents intents)
    : model_(model), intents_(std::move(intents))
{
    setOpaque(true);
}

void AnalyzerControl::Refresh()
{
    renderer_.UpdateCurves(model_.Curves(), model_.AllBanksCurve(), model_.CurvesRevision());
    repaint();
}

void AnalyzerControl::Clear()
{
    model_.Clear();
    renderer_.UpdateReference({});
    ResetInteraction();
}

void AnalyzerControl::ResetInteraction()
{
    dragging_ = false;
    qDragging_ = false;
    selected_ = -1;
    Refresh();
}

void AnalyzerControl::SetSampleRate(double sampleRate)
{
    model_.SetSampleRate(sampleRate);
    renderer_.UpdateReference(model_.ReferenceSpectrum());
    Refresh();
}

void AnalyzerControl::SetGroupScope(bool groupScope)
{
    if (model_.GroupScope() == groupScope)
    {
        return;
    }
    model_.SetGroupScope(groupScope);
    repaint();
}

void AnalyzerControl::FinishPreview()
{
    model_.FinishPreview();
    dragging_ = false;
    qDragging_ = false;
    Refresh();
}

void AnalyzerControl::ApplySpectrum(features::equalizer::clients::AnalyzerSpectrum spectrum)
{
    model_.ApplySpectrum(std::move(spectrum));
    renderer_.UpdateSpectrum(model_.SourceSpectrum(), model_.GroupSpectrum());
    renderer_.UpdateReference(model_.ReferenceSpectrum());
    repaint(Plot().getSmallestIntegerContainer());
}

void AnalyzerControl::ApplyBankSnapshots(features::equalizer::clients::BankSnapshots banks)
{
    model_.ApplyBankSnapshots(std::move(banks));
    renderer_.UpdateCurves(model_.Curves(), model_.AllBanksCurve(), model_.CurvesRevision());
    repaint(Plot().getSmallestIntegerContainer());
}

void AnalyzerControl::SetOverlays(bool reference, bool group, bool allBanks)
{
    showReference_ = reference;
    showGroup_ = group;
    showAllBanks_ = allBanks;
    repaint(Plot().getSmallestIntegerContainer());
}

juce::Rectangle<float> AnalyzerControl::Plot() const
{
    return { 8.0F, 0.0F, static_cast<float>(getWidth()) - 16.0F, static_cast<float>(getHeight()) };
}

juce::Point<float> AnalyzerControl::HandlePosition(std::size_t index) const
{
    return AnalyzerRenderer::HandlePosition(model_.Filters()[index], Plot());
}

void AnalyzerControl::SetGroupColor(juce::Colour color)
{
    if (groupColor_ == color)
    {
        return;
    }
    groupColor_ = color;
    repaint();
}

void AnalyzerControl::paint(juce::Graphics& graphics)
{
    renderer_.Draw(graphics, { Plot(), model_.Filters(), groupColor_, selected_, model_.Ready(),
        model_.GroupScope(), model_.BankActive(), model_.HasSourceSpectrum(), showGroup_ && model_.HasGroupSpectrum(),
        showAllBanks_, showReference_ });
}

void AnalyzerControl::resized()
{
    const auto plot = Plot();
    renderer_.SetPlot(plot);
    model_.SetSpectrumResolution(static_cast<std::size_t>(std::max(0.0F, std::ceil(plot.getWidth()))) + 1);
    renderer_.UpdateCurves(model_.Curves(), model_.AllBanksCurve(), model_.CurvesRevision());
    renderer_.UpdateSpectrum(model_.SourceSpectrum(), model_.GroupSpectrum());
    renderer_.UpdateReference(model_.ReferenceSpectrum());
}
void AnalyzerControl::mouseDown(const juce::MouseEvent& event)
{
    if (!model_.Ready() || !isEnabled() || dragging_ || !model_.PreviewValues().empty() || !event.mods.isLeftButtonDown())
    {
        return;
    }
    int hit = -1;
    float distance = 12;
    for (std::size_t index = 0; index < model_.Filters().size(); ++index)
    {
        const float next = HandlePosition(index).getDistanceFrom(event.position);
        if (next < distance)
        {
            distance = next;
            hit = static_cast<int>(index);
        }
    }
    if (hit < 0)
    {
        return;
    }
    const auto path = model_.FilterPath(static_cast<std::size_t>(hit));
    if (event.getNumberOfClicks() > 1)
    {
        intents_.reset(path);
        return;
    }
    if (event.mods.isCtrlDown())
    {
        intents_.bypass(path + ".bypass", model_.Filters()[static_cast<std::size_t>(hit)].active);
        return;
    }
    const auto gainRange = model_.Range(path + ".gain");
    bool editable = gainRange.first != gainRange.second;
    const auto filter = model_.Filters()[static_cast<std::size_t>(hit)];
    if (filter.type != features::equalizer::presentation::AnalyzerFilterType::Gain)
    {
        const auto frequencyRange = model_.Range(path + ".frequency");
        editable = editable || frequencyRange.first != frequencyRange.second;
    }
    if (filter.type == features::equalizer::presentation::AnalyzerFilterType::Bell)
    {
        const auto qRange = model_.Range(path + ".q");
        editable = editable || qRange.first != qRange.second;
    }
    if (!editable)
    {
        return;
    }
    selected_ = hit;
    model_.BeginPreview(static_cast<std::size_t>(hit));
    if (!intents_.begin(path, model_.PreviewValues()))
    {
        model_.FinishPreview();
        return;
    }
    dragging_ = true;
    qDragging_ = false;

    lastDragY_ = event.position.y;
    repaint();
}

void AnalyzerControl::mouseDrag(const juce::MouseEvent& event)
{
    if (!dragging_ || !model_.Ready() || !isEnabled())
    {
        return;
    }
    const auto index = static_cast<std::size_t>(selected_);
    if (event.mods.isAltDown() &&
        model_.Filters()[index].type == features::equalizer::presentation::AnalyzerFilterType::Bell)
    {
        if (!qDragging_)
        {
            qDragging_ = true;
            lastDragY_ = event.position.y;
            return;
        }
        const double delta = lastDragY_ - event.position.y;
        lastDragY_ = event.position.y;
        model_.ChangeQ(index, delta);
    }
    else
    {
        const auto plot = Plot();
        if (plot.getWidth() <= 0 || plot.getHeight() <= 0)
        {
            return;
        }
        model_.Move(index,
            static_cast<double>((event.position.x - plot.getX()) / plot.getWidth()),
            static_cast<double>((event.position.y - plot.getY()) / plot.getHeight()));
    }
    Refresh();
    intents_.change(model_.PreviewValues());
}

void AnalyzerControl::mouseUp(const juce::MouseEvent&)
{
    if (dragging_)
    {
        dragging_ = false;
        qDragging_ = false;
        intents_.end(model_.PreviewValues());
    }
}

} // namespace consolidator::vst3::features::equalizer::controls
