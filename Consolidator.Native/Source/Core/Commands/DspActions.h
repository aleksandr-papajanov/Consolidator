#pragma once

#include <variant>

#include "Core/Parameters/ParameterRoute.h"

namespace consolidator::core
{

// Non-parameter DSP commands (reset, etc.)

struct ResetSaturator
{
};

struct ResetCompressor
{
};

struct ResetEqualizer
{
};

struct ResetEqFilter
{
    dsp::FilterId FilterId;
};

struct ResetGain
{
    dsp::DeviceId gainId;
};

using DspAction = std::variant<
    ResetSaturator,
    ResetCompressor,
    ResetEqualizer,
    ResetEqFilter,
    ResetGain
>;

} // namespace consolidator::core
