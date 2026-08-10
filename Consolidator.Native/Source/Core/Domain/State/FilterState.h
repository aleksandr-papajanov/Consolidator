#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Core/Domain/State/ParameterState.h"

namespace consolidator::dsp
{

struct FilterState
{
    ParameterState<float> frequencyHz{
        ParameterId::Frequency,
        static_cast<float>(core::settings::FilterDefaults::kDefaultFrequencyHz),
        static_cast<float>(core::settings::FilterDefaults::kMinFrequencyHz),
        static_cast<float>(core::settings::FilterDefaults::kMaxFrequencyHz)};
    
    ParameterState<float> q{
        ParameterId::Q,
        static_cast<float>(core::settings::FilterDefaults::kDefaultQ),
        static_cast<float>(core::settings::FilterDefaults::kMinQ),
        static_cast<float>(core::settings::FilterDefaults::kMaxQ)};

    ParameterState<float> gainDb{
        ParameterId::Gain,
        static_cast<float>(core::settings::FilterDefaults::kDefaultGainDb),
        static_cast<float>(core::settings::FilterDefaults::kMinGainDb),
        static_cast<float>(core::settings::FilterDefaults::kMaxGainDb)};
        
    ParameterState<bool> bypass{
        ParameterId::Bypass,
        false};
};

} // namespace consolidator::dsp
