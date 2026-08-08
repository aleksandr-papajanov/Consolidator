#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Core/Parameters/DspParameter.h"

namespace consolidator::dsp
{

struct FilterState
{
    DspParameter<float> frequencyHz{
        ParameterId::Frequency,
        static_cast<float>(core::settings::FilterDefaults::kDefaultFrequencyHz),
        static_cast<float>(core::settings::FilterDefaults::kMinFrequencyHz),
        static_cast<float>(core::settings::FilterDefaults::kMaxFrequencyHz)};
    
    DspParameter<float> q{
        ParameterId::Q,
        static_cast<float>(core::settings::FilterDefaults::kDefaultQ),
        static_cast<float>(core::settings::FilterDefaults::kMinQ),
        static_cast<float>(core::settings::FilterDefaults::kMaxQ)};

    DspParameter<float> gainDb{
        ParameterId::Gain,
        static_cast<float>(core::settings::FilterDefaults::kDefaultGainDb),
        static_cast<float>(core::settings::FilterDefaults::kMinGainDb),
        static_cast<float>(core::settings::FilterDefaults::kMaxGainDb)};
        
    DspParameter<bool> bypass{
        ParameterId::Bypass,
        false};
};

} // namespace consolidator::dsp
