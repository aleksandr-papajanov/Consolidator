#pragma once

#include <array>
#include <functional>
#include <memory>

#include <JuceHeader.h>

#include "Shared/Controls/MarkerRowControl.h"

namespace consolidator::vst3::features::equalizer::controls
{

class AnalyzerOverlayList final : public juce::Component
{
public:
    explicit AnalyzerOverlayList(std::function<void(bool, bool, bool)> changed);
    void resized() override;

private:
    std::function<void(bool, bool, bool)> changed_;
    std::array<bool, 3> visible_{ true, true, true };
    std::array<std::unique_ptr<shared::controls::MarkerRowControl>, 3> rows_;
};

} // namespace consolidator::vst3::features::equalizer::controls
