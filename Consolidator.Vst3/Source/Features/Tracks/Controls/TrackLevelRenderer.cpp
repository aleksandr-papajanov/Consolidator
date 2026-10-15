#include "Features/Tracks/Controls/TrackLevelRenderer.h"

#include "Shared/Theme/UiColors.h"

namespace consolidator::vst3::features::tracks::controls
{

juce::Rectangle<int> TrackLevelRenderer::Draw(juce::Graphics& graphics,
    juce::Rectangle<int> bounds, double alpha)
{
    constexpr int markerSize = 9;
    constexpr int labelGap = 4;
    const auto marker = bounds.removeFromLeft(markerSize).withSizeKeepingCentre(markerSize, markerSize);
    graphics.setColour(shared::ui::kActive.withAlpha(static_cast<float>(alpha)));
    graphics.fillEllipse(marker.toFloat());
    return bounds.withTrimmedLeft(labelGap);
}

} // namespace consolidator::vst3::features::tracks::controls
