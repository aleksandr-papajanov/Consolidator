#include "Features/Equalizer/Controls/AnalyzerOverlayList.h"

#include <utility>

#include "Features/Equalizer/Presentation/MarkerListRow.h"

namespace consolidator::vst3::features::equalizer::controls
{

namespace
{
constexpr std::array<const char*, 3> kLabels{ "Reference", "Group", "All Banks" };
}

AnalyzerOverlayList::AnalyzerOverlayList(std::function<void(bool, bool, bool)> changed)
    : changed_(std::move(changed))
{
    list_.setRowHeight(shared::ui::kBankCellSize);
    list_.setColour(juce::ListBox::backgroundColourId, shared::ui::kAnalyzerBackground);
    list_.setOutlineThickness(0);
    addAndMakeVisible(list_);
}

int AnalyzerOverlayList::getNumRows() { return static_cast<int>(visible_.size()); }

void AnalyzerOverlayList::paintListBoxItem(int row, juce::Graphics& graphics, int width, int height, bool)
{
    if (row >= 0 && row < getNumRows())
    {
        presentation::DrawMarkerListRow(graphics, width, height, kLabels[static_cast<std::size_t>(row)],
            visible_[static_cast<std::size_t>(row)], isEnabled());
    }
}

void AnalyzerOverlayList::listBoxItemClicked(int row, const juce::MouseEvent& event)
{
    if (!isEnabled() || !event.mods.isLeftButtonDown() || row < 0 || row >= getNumRows())
    {
        return;
    }
    auto& visible = visible_[static_cast<std::size_t>(row)];
    visible = !visible;
    list_.repaint();
    changed_(visible_[0], visible_[1], visible_[2]);
}

void AnalyzerOverlayList::resized() { list_.setBounds(getLocalBounds()); }
void AnalyzerOverlayList::enablementChanged() { list_.repaint(); }

} // namespace consolidator::vst3::features::equalizer::controls
