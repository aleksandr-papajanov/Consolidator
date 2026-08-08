#pragma once

#include "Core/Parameters/ParameterRoute.h"
#include "Core/Parameters/ParameterValue.h"

namespace consolidator::dsp
{

struct RoutedParameterChange
{
    ParameterRoute route;
    ParameterValue value;
};

} // namespace consolidator::dsp