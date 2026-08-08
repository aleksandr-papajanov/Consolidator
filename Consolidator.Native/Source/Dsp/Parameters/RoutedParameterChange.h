#pragma once

#include "Dsp/Parameters/ParameterRoute.h"
#include "Dsp/Parameters/ParameterValue.h"

namespace consolidator::dsp
{

struct RoutedParameterChange
{
    ParameterRoute route;
    ParameterValue value;
};

} // namespace consolidator::dsp
