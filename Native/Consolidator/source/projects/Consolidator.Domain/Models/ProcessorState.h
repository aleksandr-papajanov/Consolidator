#pragma once

#include "Settings/CompressorOptions.h"
#include "Settings/GainOptions.h"
#include "Settings/SaturatorOptions.h"
#include "Models/DetectorFilterState.h"

#include <array>
#include <string>

namespace consolidator::models {

enum class GainStage {
    Input,
    Output
};

struct GainState {
    double gainDb = settings::GainOptions::DefaultGainDb;
    std::string linkId;
};

struct CompressorState {
    double attackMs = settings::CompressorOptions::DefaultAttackMs;
    double releaseMs = settings::CompressorOptions::DefaultReleaseMs;
    double inputDb = settings::CompressorOptions::DefaultInputDb;
    double outputDb = settings::CompressorOptions::DefaultOutputDb;
    double mix = settings::CompressorOptions::DefaultMix;
    long mode = settings::CompressorOptions::DefaultMode;
    long detectorListen = 0;
    bool bypass = false;
    std::string linkId;
    std::array<DetectorFilterState, 2> detectorFilters{ DetectorFilterState{ 1 }, DetectorFilterState{ 2 } };
};

struct SaturatorState {
    double inputDb = settings::SaturatorOptions::DefaultInputDb;
    double outputDb = settings::SaturatorOptions::DefaultOutputDb;
    long mode = settings::SaturatorOptions::DefaultMode;
    long detectorListen = 0;
    bool bypass = false;
    std::string linkId;
    std::array<DetectorFilterState, 2> detectorFilters{ DetectorFilterState{ 1 }, DetectorFilterState{ 2 } };
};

struct ProcessorState {
    GainState inputGain;
    CompressorState compressor;
    SaturatorState saturator;
    GainState outputGain;
};

} // namespace consolidator::models
