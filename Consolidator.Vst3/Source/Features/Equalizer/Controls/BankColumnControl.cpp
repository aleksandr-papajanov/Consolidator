#include "Features/Equalizer/Controls/BankColumnControl.h"

#include <cmath>
#include <utility>

#include "Features/Tracks/Controls/BankCellRenderer.h"
#include "Shared/Theme/UiColors.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::equalizer::controls
{

BankColumnControl::BankColumnControl(tracks::viewmodels::TracksViewModel& model,
    std::function<void(const tracks::bindings::TrackIntent&)> emit)
    : model_(model), interaction_(model, std::move(emit))
{
    setOpaque(true);
}

void BankColumnControl::paint(juce::Graphics& graphics)
{
    graphics.fillAll(shared::ui::kAnalyzerBackground);
    graphics.setFont(shared::ui::ControlFont());
    const auto instance = model_.FocusedInstance();
    if (model_.FindBank(instance, 0) == nullptr)
    {
        return;
    }
    for (int index = 0; index < 7; ++index)
    {
        const auto* bank = model_.FindBank(instance, index);
        tracks::controls::BankCellRenderer::Draw(graphics, { 0, index * shared::ui::kBankCellSize,
            shared::ui::kBankCellSize, shared::ui::kBankCellSize }, model_, instance, *bank);
    }
    graphics.setColour(shared::ui::kLines);
    for (int index = 0; index <= 7; ++index)
    {
        graphics.drawHorizontalLine(index * shared::ui::kBankCellSize, 0, shared::ui::kBankCellSize + 1);
    }
    graphics.drawVerticalLine(0, 0, 7 * shared::ui::kBankCellSize + 1);
    graphics.drawVerticalLine(shared::ui::kBankCellSize, 0, 7 * shared::ui::kBankCellSize + 1);
}

void BankColumnControl::SelectAt(const juce::MouseEvent& event, bool dragging)
{
    if (!isEnabled() || event.position.x < 0 || event.position.x >= shared::ui::kBankCellSize
        || event.position.y < 0 || event.position.y >= 7 * shared::ui::kBankCellSize)
    {
        return;
    }
    interaction_.Handle(model_.FocusedInstance(),
        static_cast<int>(std::floor(event.position.y / shared::ui::kBankCellSize)), event, dragging);
    repaint();
}

void BankColumnControl::mouseDown(const juce::MouseEvent& event)
{
    interaction_.BeginGesture();
    SelectAt(event, false);
}

void BankColumnControl::mouseDrag(const juce::MouseEvent& event)
{
    if (event.mods.isShiftDown())
    {
        SelectAt(event, true);
    }
}

} // namespace consolidator::vst3::features::equalizer::controls
