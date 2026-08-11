#pragma once

#include <array>

#include "Core/Domain/State/DspStates.h"
#include "Core/Domain/State/InstanceState.h"

// Complete user-facing state tree for one instance's DSP chain.
namespace consolidator::core
{

struct ChainState
{
    dsp::GainState inputGain;
    dsp::SaturatorState saturator;
    dsp::CompressorState compressor;
    dsp::EqualizerState equalizer;
    std::array<dsp::EqualizerBankState, InstanceState::kBankCount> equalizers;
    dsp::GainState outputGain;
};

} // namespace consolidator::core
