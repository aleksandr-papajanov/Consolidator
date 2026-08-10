#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Core/Domain/State/ParameterState.h"

namespace consolidator::dsp
{

struct SaturatorState
{
    ParameterState<float> drive{
        ParameterId::Drive,
        static_cast<float>(core::settings::SaturatorDefaults::kDefaultDrive),
        static_cast<float>(core::settings::SaturatorDefaults::kMinDrive),
        static_cast<float>(core::settings::SaturatorDefaults::kMaxDrive)};

    ParameterState<float> outputDb{
        ParameterId::Gain,
        static_cast<float>(core::settings::SaturatorDefaults::kDefaultOutputDb),
        static_cast<float>(core::settings::SaturatorDefaults::kMinOutputDb),
        static_cast<float>(core::settings::SaturatorDefaults::kMaxOutputDb)};
        
    ParameterState<float> mix{
        ParameterId::Mix,
        static_cast<float>(core::settings::SaturatorDefaults::kDefaultMix),
        static_cast<float>(core::settings::SaturatorDefaults::kMinMix),
        static_cast<float>(core::settings::SaturatorDefaults::kMaxMix)};

    ParameterState<float> detectorAmount{
        ParameterId::Type,
        1.0f,
        0.0f,
        8.0f};

    ParameterState<bool> bypass{
        ParameterId::Bypass,
        false};
};

} // namespace consolidator::dsp
