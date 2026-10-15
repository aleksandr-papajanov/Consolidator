#pragma once

#include <cstdint>
#include <functional>
#include <set>
#include <utility>

#include <JuceHeader.h>

#include "Features/Tracks/Bindings/TrackIntent.h"
#include "Features/Tracks/ViewModels/TracksViewModel.h"

namespace consolidator::vst3::features::tracks::controllers
{

class BankCellInteraction final
{
public:
    BankCellInteraction(viewmodels::TracksViewModel& model,
        std::function<void(const bindings::TrackIntent&)> emit);
    void BeginGesture();
    void Handle(std::uint64_t instance, int bank, const juce::MouseEvent& event, bool dragging);

private:
    viewmodels::TracksViewModel& model_;
    std::function<void(const bindings::TrackIntent&)> emit_;
    std::set<std::pair<std::uint64_t, int>> visited_;
};

} // namespace consolidator::vst3::features::tracks::controllers
