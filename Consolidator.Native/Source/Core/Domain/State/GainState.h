#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Core/Domain/State/ParameterState.h"

namespace consolidator::dsp
{

struct GainState
{
    ParameterState<float> gainDb{
        ParameterId::Gain,
        static_cast<float>(core::settings::GainDefaults::kDefaultGainDb),
        static_cast<float>(core::settings::GainDefaults::kMinGainDb),
        static_cast<float>(core::settings::GainDefaults::kMaxGainDb)};
        
    ParameterState<bool> bypass{
        ParameterId::Bypass,
        false};
};

} // namespace consolidator::dsp
