#pragma once

#include "Core/Parameters/DspParameter.h"

namespace consolidator::dsp
{

struct EqualizerState
{
    DspParameter<bool> bypass{
        ParameterId::Bypass,
        false};
};

} // namespace consolidator::dsp
