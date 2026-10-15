#pragma once

#include <array>
#include <string_view>

#include "Shared/Controls/Parameter/Presentation/ParameterPresentation.h"

namespace consolidator::vst3::features::channelstrip::presentation
{

inline constexpr std::array<std::string_view, 9> kGainStagingPaths{
    "gain_staging.input_leveler", "gain_staging.input_level", "gain_staging.input_target",
    "gain_staging.output_leveler", "gain_staging.output_level", "gain_staging.output_target",
    "gain_staging.limit", "gain_staging.width", "gain_staging.bypass"
};
inline constexpr std::array<std::string_view, 5> kSaturatorPaths{
    "saturator.drive", "saturator.curve", "saturator.split", "saturator.output", "saturator.bypass"
};
inline constexpr std::array<std::string_view, 7> kCompressorPaths{
    "compressor.attack", "compressor.sustain", "compressor.compression", "compressor.character",
    "compressor.parallel", "compressor.output", "compressor.bypass"
};
inline constexpr std::array<std::string_view, 3> kPolishPaths{ "polish.thick", "polish.air", "polish.bypass" };
inline constexpr std::array<std::string_view, 3> kCharacterLabels{ "PUNCH", "TIGHT", "SMOOTH" };
inline constexpr std::array<shared::controls::parameter::presentation::ParameterPresentation, 4> kSections{
    shared::controls::parameter::presentation::ParameterPresentation{ "gain_staging", kGainStagingPaths, {}, {} },
    shared::controls::parameter::presentation::ParameterPresentation{ "saturator", kSaturatorPaths, {}, {} },
    shared::controls::parameter::presentation::ParameterPresentation{
        "compressor", kCompressorPaths, "compressor.character", kCharacterLabels },
    shared::controls::parameter::presentation::ParameterPresentation{ "polish", kPolishPaths, {}, {} }
};

} // namespace consolidator::vst3::features::channelstrip::presentation
