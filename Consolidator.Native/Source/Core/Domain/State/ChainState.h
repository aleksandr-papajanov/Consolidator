#pragma once

#include <array>

#include "Core/Domain/State/DspStates.h"
#include "Core/Domain/State/InstanceState.h"

// Complete user-facing state tree for one instance's DSP chain.
namespace consolidator::core
{

struct EqualizerBankState
{
    dsp::EqualizerState state;
    std::array<dsp::FilterState, 7> filters;
};

struct ChainState
{
    dsp::GainState inputGain;
    dsp::SaturatorState saturator;
    dsp::CompressorState compressor;
    std::array<EqualizerBankState, InstanceState::kBankCount> equalizers;
    dsp::GainState outputGain;
};

} // namespace consolidator::core
