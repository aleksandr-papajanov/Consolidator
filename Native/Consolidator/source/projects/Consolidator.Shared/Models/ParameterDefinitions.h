#pragma once

#include "ParameterRange.h"

#include <map>
#include <string>

namespace consolidator::models {

using ParameterDefinitionTable = std::map<std::string, ParameterRange, std::less<>>;

inline const ParameterDefinitionTable& ParameterDefinitions() {
    static const ParameterDefinitionTable definitions{
        { "input_gain.gain", { -36.0, 36.0, ParameterScale::Linear } },
        { "output_gain.gain", { -36.0, 36.0, ParameterScale::Linear } },
        { "compressor.attack", { 0.1, 500.0, ParameterScale::Logarithmic } },
        { "compressor.release", { 5.0, 2000.0, ParameterScale::Logarithmic } },
        { "compressor.threshold", { -60.0, 0.0, ParameterScale::Linear } },
        { "compressor.output", { -36.0, 36.0, ParameterScale::Linear } },
        { "compressor.mix", { 0.0, 1.0, ParameterScale::Linear } },
        { "saturator.saturation", { 0.0, 1.0, ParameterScale::Linear } },
        { "saturator.output", { -36.0, 36.0, ParameterScale::Linear } },
        { "compressor.detector.1.gain", { -24.0, 24.0, ParameterScale::Linear } },
        { "compressor.detector.1.frequency", { 40.0, 18000.0, ParameterScale::Logarithmic } },
        { "compressor.detector.1.q", { 0.1, 10.0, ParameterScale::Logarithmic } },
        { "compressor.detector.2.gain", { -24.0, 24.0, ParameterScale::Linear } },
        { "compressor.detector.2.frequency", { 40.0, 18000.0, ParameterScale::Logarithmic } },
        { "compressor.detector.2.q", { 0.1, 10.0, ParameterScale::Logarithmic } },
        { "saturator.detector.1.gain", { -24.0, 24.0, ParameterScale::Linear } },
        { "saturator.detector.1.frequency", { 40.0, 18000.0, ParameterScale::Logarithmic } },
        { "saturator.detector.1.q", { 0.1, 10.0, ParameterScale::Logarithmic } },
        { "saturator.detector.2.gain", { -24.0, 24.0, ParameterScale::Linear } },
        { "saturator.detector.2.frequency", { 40.0, 18000.0, ParameterScale::Logarithmic } },
        { "saturator.detector.2.q", { 0.1, 10.0, ParameterScale::Logarithmic } }
    };
    return definitions;
}

} // namespace consolidator::models
