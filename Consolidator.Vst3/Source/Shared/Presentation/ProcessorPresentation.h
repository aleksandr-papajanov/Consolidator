#pragma once

#include <algorithm>
#include <array>
#include <stdexcept>
#include <string_view>

#include <JuceHeader.h>

#include "Shared/Theme/UiColors.h"

namespace consolidator::vst3::shared::presentation
{

struct ProcessorPresentation
{
    std::string_view id;
    std::string_view label;
    std::string_view listLabel;
    juce::Colour color;
};

inline const std::array<ProcessorPresentation, 6> kProcessors{
    ProcessorPresentation{ "gain_staging", "GAIN STAGING", "Gain Staging", ui::kGainStaging },
    ProcessorPresentation{ "saturator", "SATURATOR", "Saturator", ui::kSaturator },
    ProcessorPresentation{ "shaper", "SHAPER", "Shaper", ui::kShaper },
    ProcessorPresentation{ "compression", "COMPRESSION", "Compression", ui::kCompression },
    ProcessorPresentation{ "equalizer", "EQUALIZER", "Equalizer", ui::kEqualizer },
    ProcessorPresentation{ "polish", "POLISH", "Polish", ui::kPolish }
};

inline const ProcessorPresentation& ProcessorFor(std::string_view id)
{
    const auto found = std::find_if(kProcessors.begin(), kProcessors.end(), [id](const auto& processor)
    {
        return processor.id == id;
    });
    if (found == kProcessors.end())
    {
        throw std::invalid_argument("Unknown processor presentation.");
    }
    return *found;
}

} // namespace consolidator::vst3::shared::presentation
