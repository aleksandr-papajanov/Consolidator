#pragma once

#include <array>
#include <string_view>

#include "Shared/Controls/Parameter/Presentation/ParameterPresentation.h"

namespace consolidator::vst3::features::channelstrip::presentation
{

inline constexpr std::array<std::string_view, 5> kInputPaths{
    "input_gain.level", "input_gain.target", "input_gain.leveler", "input_gain.width", "input_gain.bypass"
};
inline constexpr std::array<std::string_view, 5> kSaturatorPaths{
    "saturator.drive", "saturator.curve", "saturator.split", "saturator.output", "saturator.bypass"
};
inline constexpr std::array<std::string_view, 7> kCompressorPaths{
    "compressor.attack", "compressor.sustain", "compressor.compression", "compressor.character",
    "compressor.parallel", "compressor.output", "compressor.bypass"
};
inline constexpr std::array<std::string_view, 3> kPolishPaths{ "polish.thick", "polish.air", "polish.bypass" };
inline constexpr std::array<std::string_view, 4> kOutputPaths{
    "output_gain.level", "output_gain.target", "output_gain.limiter", "output_gain.bypass"
};
inline constexpr std::array<std::string_view, 3> kCharacterLabels{ "PUNCH", "TIGHT", "SMOOTH" };
inline constexpr std::array<std::string_view, 1> kInputHistory{ "input_gain.leveler" };
inline constexpr std::array<std::string_view, 1> kSaturatorHistory{ "saturator.split" };
inline constexpr std::array<std::string_view, 2> kCompressorHistory{ "compressor.character", "compressor.parallel" };
inline constexpr std::array<std::string_view, 1> kOutputHistory{ "output_gain.limiter" };

inline constexpr std::array<shared::controls::parameter::presentation::ParameterPresentation, 5> kSections{
    shared::controls::parameter::presentation::ParameterPresentation{ "input", kInputPaths, {}, {}, kInputHistory },
    shared::controls::parameter::presentation::ParameterPresentation{ "saturator", kSaturatorPaths, {}, {}, kSaturatorHistory },
    shared::controls::parameter::presentation::ParameterPresentation{
        "compressor", kCompressorPaths, "compressor.character", kCharacterLabels, kCompressorHistory },
    shared::controls::parameter::presentation::ParameterPresentation{ "polish", kPolishPaths, {}, {}, {} },
    shared::controls::parameter::presentation::ParameterPresentation{ "output", kOutputPaths, {}, {}, kOutputHistory }
};

[[nodiscard]] inline constexpr bool UsesDiscreteHistory(std::string_view path) noexcept
{
    for (const auto& section : kSections)
    {
        for (const auto candidate : section.historyPaths)
        {
            if (path == candidate)
            {
                return true;
            }
        }
    }
    return false;
}

} // namespace consolidator::vst3::features::channelstrip::presentation
