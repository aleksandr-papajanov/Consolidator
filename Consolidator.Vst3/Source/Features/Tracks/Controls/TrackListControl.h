#pragma once

#include <cstdint>
#include <memory>
#include <functional>
#include <map>
#include <string>
#include <utility>

#include <JuceHeader.h>

#include "Features/Tracks/ViewModels/TracksViewModel.h"
#include "Features/Tracks/Bindings/TrackIntent.h"
#include "Features/Tracks/Controllers/BankCellInteraction.h"
#include "Features/Tracks/Controls/InstanceButtonsControl.h"

namespace consolidator::vst3::features::tracks::controls
{

// Ports BankManagerLayout/Interaction/HitActions and local selection/scrolling.
class TrackListControl final : public juce::Component
{
public:
    TrackListControl(features::tracks::viewmodels::TracksViewModel& model,
        std::function<void(const features::tracks::bindings::TrackIntent&)> emit);
    void Refresh();
    void RefreshPresentation();
    void paint(juce::Graphics& graphics) override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseWheelMove(const juce::MouseEvent& event, const juce::MouseWheelDetails& wheel) override;
    void resized() override;

private:
    struct Layout
    {
        int markers;
        int banks;
        int controls;
        int labelWidth;
    };
    [[nodiscard]] Layout GetLayout() const;
    void SelectAt(const juce::MouseEvent& event, bool dragging);
    void ScrollBy(float amount);
    void SyncRowButtons();
    void PositionRowButtons();

    features::tracks::viewmodels::TracksViewModel& model_;
    std::function<void(const features::tracks::bindings::TrackIntent&)> emit_;
    features::tracks::controllers::BankCellInteraction bankInteraction_;
    std::map<std::uint64_t, std::unique_ptr<InstanceButtonsControl>> rowButtons_;

    float scroll_{};
    float lastY_{};
};

} // namespace consolidator::vst3::features::tracks::controls
