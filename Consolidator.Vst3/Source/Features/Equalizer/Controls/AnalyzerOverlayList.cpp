#include "Features/Equalizer/Controls/AnalyzerOverlayList.h"

#include <utility>

#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::equalizer::controls
{

namespace
{
constexpr std::array<const char*, 3> kLabels{ "Reference", "Group", "All Banks" };
}

AnalyzerOverlayList::AnalyzerOverlayList(std::function<void(bool, bool, bool)> changed)
    : changed_(std::move(changed))
{
    for (std::size_t index = 0; index < rows_.size(); ++index)
    {
        auto row = std::make_unique<shared::controls::MarkerRowControl>(kLabels[index]);
        row->SetActive(visible_[index]);
        row->onClick = [this, index]
        {
            visible_[index] = !visible_[index];
            rows_[index]->SetActive(visible_[index]);
            changed_(visible_[0], visible_[1], visible_[2]);
        };
        addAndMakeVisible(*row);
        rows_[index] = std::move(row);
    }
}

void AnalyzerOverlayList::resized()
{
    for (std::size_t index = 0; index < rows_.size(); ++index)
    {
        rows_[index]->setBounds(0, static_cast<int>(index) * shared::ui::kBankCellSize,
            getWidth(), shared::ui::kBankCellSize);
    }
}

} // namespace consolidator::vst3::features::equalizer::controls
