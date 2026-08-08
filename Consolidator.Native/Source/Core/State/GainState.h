#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Core/Parameters/DspParameter.h"

namespace consolidator::dsp
{

struct GainState
{
    DspParameter<float> gainDb{
        ParameterId::Gain,
        static_cast<float>(core::settings::GainDefaults::kDefaultGainDb),
        static_cast<float>(core::settings::GainDefaults::kMinGainDb),
        static_cast<float>(core::settings::GainDefaults::kMaxGainDb)};
        
    DspParameter<bool> bypass{
        ParameterId::Bypass,
        false};
};

} // namespace consolidator::dsp
