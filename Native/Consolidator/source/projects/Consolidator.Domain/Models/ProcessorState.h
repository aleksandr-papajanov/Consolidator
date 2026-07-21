#pragma once

#include "Settings/CompressorOptions.h"
#include "Settings/GainOptions.h"
#include "Settings/SaturatorOptions.h"

namespace consolidator::models {

enum class GainStage {
    Input,
    Output
};

struct GainState {
    double gainDb = settings::GainOptions::DefaultGainDb;
};

struct CompressorState {
    double attackMs = settings::CompressorOptions::DefaultAttackMs;
    double releaseMs = settings::CompressorOptions::DefaultReleaseMs;
    double thresholdDb = settings::CompressorOptions::DefaultThresholdDb;
    bool bypass = false;
};

struct SaturatorState {
    double saturation = settings::SaturatorOptions::DefaultSaturation;
    bool bypass = false;
};

struct ProcessorState {
    GainState inputGain;
    CompressorState compressor;
    SaturatorState saturator;
    GainState outputGain;
};

} // namespace consolidator::models
