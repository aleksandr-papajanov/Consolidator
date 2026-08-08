#pragma once

#include "Core/Settings/DspDeviceSettings.h"
#include "Core/Parameters/DspParameter.h"

namespace consolidator::dsp
{

struct CompressorState
{
    DspParameter<float> thresholdDb{
        ParameterId::Threshold,
        static_cast<float>(core::settings::CompressorDefaults::kDefaultThresholdDb),
        static_cast<float>(core::settings::CompressorDefaults::kMinThresholdDb),
        static_cast<float>(core::settings::CompressorDefaults::kMaxThresholdDb)};

    DspParameter<float> ratio{
        ParameterId::Ratio,
        static_cast<float>(core::settings::CompressorDefaults::kDefaultRatio),
        static_cast<float>(core::settings::CompressorDefaults::kMinRatio),
        static_cast<float>(core::settings::CompressorDefaults::kMaxRatio)};

    DspParameter<float> attackMs{
        ParameterId::Attack,
        static_cast<float>(core::settings::CompressorDefaults::kDefaultAttackMs),
        static_cast<float>(core::settings::CompressorDefaults::kMinAttackMs),
        static_cast<float>(core::settings::CompressorDefaults::kMaxAttackMs)};

    DspParameter<float> releaseMs{
        ParameterId::Release,
        static_cast<float>(core::settings::CompressorDefaults::kDefaultReleaseMs),
        static_cast<float>(core::settings::CompressorDefaults::kMinReleaseMs),
        static_cast<float>(core::settings::CompressorDefaults::kMaxReleaseMs)};

    DspParameter<float> outputDb{
        ParameterId::Gain,
        static_cast<float>(core::settings::CompressorDefaults::kDefaultOutputDb),
        static_cast<float>(core::settings::CompressorDefaults::kMinOutputDb),
        static_cast<float>(core::settings::CompressorDefaults::kMaxOutputDb)};
        
    DspParameter<float> mix{
        ParameterId::Mix,
        static_cast<float>(core::settings::CompressorDefaults::kDefaultMix),
        static_cast<float>(core::settings::CompressorDefaults::kMinMix),
        static_cast<float>(core::settings::CompressorDefaults::kMaxMix)};

    DspParameter<bool> bypass{
        ParameterId::Bypass,
        false};
};

} // namespace consolidator::dsp
