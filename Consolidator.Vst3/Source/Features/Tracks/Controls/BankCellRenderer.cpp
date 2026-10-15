#include "Features/Tracks/Controls/BankCellRenderer.h"

#include <string>

#include "Shared/Theme/UiColors.h"

namespace consolidator::vst3::features::tracks::controls
{

void BankCellRenderer::Draw(juce::Graphics& graphics, juce::Rectangle<int> bounds,
    viewmodels::TracksViewModel& model, std::uint64_t instance, const shared::clients::RegistryBank& bank)
{
    const auto cell = model.BankCell(instance, bank);
    const auto color = cell.selected ? shared::ui::kProcessors[3]
        : bank.group ? shared::ui::GroupColor(*bank.group) : shared::ui::kActive;
    const auto displayColor = color.withAlpha(cell.focused ? 1.0F : 0.8F);
    if (cell.highlighted)
    {
        graphics.setColour(bank.active ? displayColor : shared::ui::kLines);
        graphics.fillRect(bounds);
    }
    graphics.setColour(cell.highlighted
        ? bank.active ? shared::ui::kBackground : shared::ui::kDisabled : displayColor);
    if (bank.group)
    {
        graphics.drawText(juce::String(viewmodels::TracksViewModel::GroupLabel(*bank.group)),
            bounds, juce::Justification::centred);
    }
    if (bank.active)
    {
        graphics.setColour(shared::ui::kBackground);
        graphics.fillRect(bounds.getX() + 1, bounds.getY() + 1, 8, 8);
        graphics.setColour(model.Feedback().Bypass(instance, "bank." + std::to_string(bank.index), bank.bypass)
            ? shared::ui::kDisabled : shared::ui::kProcessors[3]);
        graphics.fillRect(bounds.getX() + 2, bounds.getY() + 2, 6, 6);
    }
}

} // namespace consolidator::vst3::features::tracks::controls
