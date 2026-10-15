#include "Features/Equalizer/Controls/AnalyzerSourceList.h"

#include <array>
#include <utility>

#include "Features/Equalizer/Presentation/MarkerListRow.h"
#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::equalizer::controls
{

namespace
{
const std::array<std::string, 4> kPaths{ "equalizer", "gain_staging", "saturator", "compressor" };
const std::array<const char*, 4> kLabels{ "Equalizer", "Gain Staging", "Saturator", "Compressor" };
}

AnalyzerSourceList::AnalyzerSourceList(std::function<void()> changed) : changed_(std::move(changed))
{
    list_.setRowHeight(shared::ui::kBankCellSize);
    list_.setColour(juce::ListBox::backgroundColourId, shared::ui::kAnalyzerBackground);
    list_.setOutlineThickness(0);
    list_.setMultipleSelectionEnabled(false);
    list_.selectRow(0);
    addAndMakeVisible(list_);
}

const std::string& AnalyzerSourceList::ContextPath() const
{
    return kPaths.at(static_cast<std::size_t>(selected_));
}

int AnalyzerSourceList::getNumRows() { return static_cast<int>(kPaths.size()); }

void AnalyzerSourceList::paintListBoxItem(int row, juce::Graphics& graphics, int width, int height, bool)
{
    if (row < 0 || row >= getNumRows())
    {
        return;
    }
    presentation::DrawMarkerListRow(graphics, width, height, kLabels[static_cast<std::size_t>(row)],
        row == selected_, isEnabled());
}

void AnalyzerSourceList::selectedRowsChanged(int row)
{
    if (row < 0 || row == selected_)
    {
        return;
    }
    selected_ = row;
    list_.repaint();
    changed_();
}

void AnalyzerSourceList::resized() { list_.setBounds(getLocalBounds()); }
void AnalyzerSourceList::enablementChanged() { list_.repaint(); }

juce::String AnalyzerSourceList::getTooltipForRow(int row)
{
    if (row < 0 || row >= getNumRows())
    {
        return {};
    }
    return juce::String(kLabels[static_cast<std::size_t>(row)]) + (row == 0 ? "" : " Detector");
}

} // namespace consolidator::vst3::features::equalizer::controls
