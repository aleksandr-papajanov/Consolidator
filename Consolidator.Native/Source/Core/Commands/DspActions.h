#pragma once

#include <variant>

#include "Dsp/Parameters/ParameterAddress.h"

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
    dsp::EqFilterId filterId;
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