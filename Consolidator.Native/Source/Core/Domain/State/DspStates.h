#pragma once

#include <array>

#include "Core/Domain/State/ParameterState.h"
#include "Core/Domain/State/StateMarker.h"

// Passive user-facing state containers for the DSP device hierarchy.
namespace consolidator::dsp
{

struct GainState
{
    ParameterState<float> gainDb;
    StateMarker<bool> bypass;
};

struct FilterState
{
    ParameterState<float> frequencyHz;
    ParameterState<float> q;
    ParameterState<float> gainDb;
    StateMarker<bool> bypass;
    StateMarker<bool> solo;
};

struct EqualizerState
{
    StateMarker<bool> bypass;
    StateMarker<bool> solo;
};

struct EqualizerBankState
{
    StateMarker<bool> bypass;
    StateMarker<bool> solo;
    std::array<FilterState, 7> filters;
};

struct DetectorState
{
    std::array<FilterState, 2> filters;
    StateMarker<bool> listen;
};

struct SaturatorState
{
    ParameterState<float> drive;
    ParameterState<float> outputDb;
    ParameterState<float> mix;
    ParameterState<float> detectorAmount;
    StateMarker<bool> bypass;
    StateMarker<bool> solo;
    DetectorState detector;
};

struct CompressorState
{
    ParameterState<float> thresholdDb;
    ParameterState<float> ratio;
    ParameterState<float> attackMs;
    ParameterState<float> releaseMs;
    ParameterState<float> outputDb;
    ParameterState<float> mix;
    StateMarker<bool> bypass;
    StateMarker<bool> solo;
    DetectorState detector;
};

} // namespace consolidator::dsp
