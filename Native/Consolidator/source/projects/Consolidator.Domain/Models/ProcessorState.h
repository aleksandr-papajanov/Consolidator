#pragma once

#include "Settings/CompressorOptions.h"
#include "Settings/GainOptions.h"
#include "Settings/SaturatorOptions.h"
#include "Models/DetectorFilterState.h"

#include <array>
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
    double outputDb = settings::CompressorOptions::DefaultOutputDb;
    double mix = settings::CompressorOptions::DefaultMix;
    long detectorListen = 0;
    bool bypass = false;
    std::array<DetectorFilterState, 2> detectorFilters{ DetectorFilterState{ 1 }, DetectorFilterState{ 2 } };
};

struct SaturatorState {
    double saturation = settings::SaturatorOptions::DefaultSaturation;
    double outputDb = settings::SaturatorOptions::DefaultOutputDb;
    long detectorListen = 0;
    bool bypass = false;
    std::array<DetectorFilterState, 2> detectorFilters{ DetectorFilterState{ 1 }, DetectorFilterState{ 2 } };
};

struct ProcessorState {
    GainState inputGain;
    CompressorState compressor;
    SaturatorState saturator;
    GainState outputGain;
};

} // namespace consolidator::models
