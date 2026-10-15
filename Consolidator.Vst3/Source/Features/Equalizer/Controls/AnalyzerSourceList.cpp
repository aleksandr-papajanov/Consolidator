#include "Features/Equalizer/Controls/AnalyzerSourceList.h"

#include <array>
#include <utility>

#include "Shared/Theme/UiMetrics.h"

namespace consolidator::vst3::features::equalizer::controls
{

namespace
{
const std::array<std::string, 5> kPaths{ "equalizer", "gain_staging.input_detector",
    "saturator", "compression", "gain_staging.output_detector" };
const std::array<const char*, 5> kLabels{ "Equalizer", "Input", "Saturator", "Compression", "Output" };
}

AnalyzerSourceList::AnalyzerSourceList(std::function<void()> changed) : changed_(std::move(changed))
{
    for (std::size_t index = 0; index < rows_.size(); ++index)
    {
        const juce::String label(kLabels[index]);
        auto row = std::make_unique<shared::controls::MarkerRowControl>(label);
        row->setTooltip(label + (index == 0 ? "" : " Detector"));
        row->SetActive(index == 0);
        row->onClick = [this, index]
        {
            if (selected_ == static_cast<int>(index))
            {
                return;
            }
            selected_ = static_cast<int>(index);
            for (std::size_t item = 0; item < rows_.size(); ++item)
            {
                rows_[item]->SetActive(item == index);
            }
            changed_();
        };
        addAndMakeVisible(*row);
        rows_[index] = std::move(row);
    }
}

const std::string& AnalyzerSourceList::ContextPath() const
{
    return kPaths.at(static_cast<std::size_t>(selected_));
}

void AnalyzerSourceList::resized()
{
    for (std::size_t index = 0; index < rows_.size(); ++index)
    {
        rows_[index]->setBounds(0, static_cast<int>(index) * shared::ui::kBankCellSize,
            getWidth(), shared::ui::kBankCellSize);
    }
}

} // namespace consolidator::vst3::features::equalizer::controls
