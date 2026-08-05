#pragma once

#include "Dsp/Parameters/ParameterAddress.h"
#include "Dsp/Parameters/ParameterValue.h"

namespace consolidator::dsp
{

struct ParameterChange
{
    ParameterAddress address;
    ParameterValue value;
};

} // namespace consolidator::dsp
