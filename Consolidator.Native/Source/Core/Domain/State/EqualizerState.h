#pragma once

#include "Core/Domain/State/ParameterState.h"

namespace consolidator::dsp
{

struct EqualizerState
{
    ParameterState<bool> bypass{
        ParameterId::Bypass,
        false};
};

} // namespace consolidator::dsp
