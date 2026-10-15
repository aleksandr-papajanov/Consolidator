#pragma once

#include <cstddef>

#include <JuceHeader.h>

#include "Shared/Clients/AnalyzerData.h"
#include "Features/Equalizer/Bindings/AnalyzerIntents.h"
#include "Features/Equalizer/ViewModels/AnalyzerViewModel.h"
#include "Features/Equalizer/Controls/AnalyzerRenderer.h"

namespace consolidator::vst3::features::equalizer::controls
{

// Pointer interaction and invalidation over the binding-owned presentation model.
class AnalyzerControl final : public juce::Component
{
public:
    AnalyzerControl(features::equalizer::viewmodels::AnalyzerViewModel& model, features::equalizer::bindings::AnalyzerIntents intents);
    void Refresh();
    void ResetInteraction();
    void Clear();
    void SetSampleRate(double sampleRate);
    void SetGroupColor(juce::Colour color);
    void SetGroupScope(bool groupScope);
    void FinishPreview();
    void ApplySpectrum(shared::clients::AnalyzerSpectrum spectrum);
    void SetOverlays(bool reference, bool group, bool allBanks);
    void ApplyBankSnapshots(shared::clients::BankSnapshots banks);
    void paint(juce::Graphics& graphics) override;
    void resized() override;
    void mouseDown(const juce::MouseEvent& event) override;
    void mouseDrag(const juce::MouseEvent& event) override;
    void mouseUp(const juce::MouseEvent& event) override;

private:
    [[nodiscard]] juce::Rectangle<float> Plot() const;
    [[nodiscard]] juce::Point<float> HandlePosition(std::size_t index) const;
    features::equalizer::viewmodels::AnalyzerViewModel& model_;
    features::equalizer::bindings::AnalyzerIntents intents_;
    AnalyzerRenderer renderer_;
    juce::Colour groupColor_;
    bool dragging_{};
    bool qDragging_{};
    bool showReference_{ true };
    bool showGroup_{ true };
    bool showAllBanks_{ true };
    float lastDragY_{};
    int selected_{ -1 };
};

} // namespace consolidator::vst3::features::equalizer::controls
