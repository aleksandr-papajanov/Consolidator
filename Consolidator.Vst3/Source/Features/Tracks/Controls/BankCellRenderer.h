#pragma once

#include <cstdint>

#include <JuceHeader.h>

#include "Features/Tracks/ViewModels/TracksViewModel.h"

namespace consolidator::vst3::features::tracks::controls
{

class BankCellRenderer final
{
public:
    static void Draw(juce::Graphics& graphics, juce::Rectangle<int> bounds,
        viewmodels::TracksViewModel& model, std::uint64_t instance, const shared::clients::RegistryBank& bank);
};

} // namespace consolidator::vst3::features::tracks::controls
