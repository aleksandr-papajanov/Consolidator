#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Core/Parameters/DspParameter.h"

namespace consolidator::dsp
{

struct SaturatorState
{
    DspParameter<float> drive{
        ParameterId::Drive,
        static_cast<float>(core::settings::SaturatorDefaults::kDefaultDrive),
        static_cast<float>(core::settings::SaturatorDefaults::kMinDrive),
        static_cast<float>(core::settings::SaturatorDefaults::kMaxDrive)};

    DspParameter<float> outputDb{
        ParameterId::Gain,
        static_cast<float>(core::settings::SaturatorDefaults::kDefaultOutputDb),
        static_cast<float>(core::settings::SaturatorDefaults::kMinOutputDb),
        static_cast<float>(core::settings::SaturatorDefaults::kMaxOutputDb)};
        
    DspParameter<float> mix{
        ParameterId::Mix,
        static_cast<float>(core::settings::SaturatorDefaults::kDefaultMix),
        static_cast<float>(core::settings::SaturatorDefaults::kMinMix),
        static_cast<float>(core::settings::SaturatorDefaults::kMaxMix)};

    DspParameter<float> detectorAmount{
        ParameterId::Type,
        1.0f,
        0.0f,
        8.0f};

    DspParameter<bool> bypass{
        ParameterId::Bypass,
        false};
};

} // namespace consolidator::dsp
