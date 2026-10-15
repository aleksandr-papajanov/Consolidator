#pragma once

#include <array>
#include <functional>
#include <memory>
#include <string>

#include <JuceHeader.h>

#include "Shared/Controls/MarkerRowControl.h"

namespace consolidator::vst3::features::equalizer::controls
{

class AnalyzerSourceList final : public juce::Component
{
public:
    explicit AnalyzerSourceList(std::function<void()> changed);
    [[nodiscard]] const std::string& ContextPath() const;
    void resized() override;

private:
    std::function<void()> changed_;
    int selected_{};
    std::array<std::unique_ptr<shared::controls::MarkerRowControl>, 4> rows_;
};

} // namespace consolidator::vst3::features::equalizer::controls
