#pragma once

#include <array>
#include <cstdint>
#include <functional>
#include <memory>

#include <JuceHeader.h>

#include "Features/Tracks/Bindings/TrackIntent.h"
#include "Features/Tracks/ViewModels/TracksViewModel.h"

namespace consolidator::vst3::features::tracks::controls
{

class InstanceButtonsControl final : public juce::Component
{
public:
    InstanceButtonsControl(viewmodels::TracksViewModel& model,
        std::function<void(const bindings::TrackIntent&)> emit, std::uint64_t instance);
    void SetInstance(std::uint64_t instance);
    void SetVerticalConnections(bool above, bool below);
    void RefreshPresentation();
    void resized() override;

private:
    void Click(std::size_t index);
    viewmodels::TracksViewModel& model_;
    std::function<void(const bindings::TrackIntent&)> emit_;
    std::uint64_t instance_;
    int verticalEdges_{};
    std::array<std::unique_ptr<juce::TextButton>, 4> buttons_;
};

} // namespace consolidator::vst3::features::tracks::controls
