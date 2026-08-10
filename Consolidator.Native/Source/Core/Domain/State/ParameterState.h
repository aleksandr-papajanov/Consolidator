#pragma once

#include "Core/Domain/Ids/DspIds.h"

namespace consolidator::dsp
{

template <typename T>
struct ParameterState
{
    ParameterId id;
    T value;
    T minimum;
    T maximum;
};

template <>
struct ParameterState<bool>
{
    ParameterId id;
    bool value;
};

} // namespace consolidator::dsp
