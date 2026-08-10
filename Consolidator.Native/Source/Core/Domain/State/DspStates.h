#pragma once

#include <array>

#include "Core/Domain/State/ParameterState.h"

namespace consolidator::dsp
{

struct GainState
{
    ParameterState<float> gainDb;
    ParameterState<bool> bypass;
};

struct FilterState
{
    ParameterState<float> frequencyHz;
    ParameterState<float> q;
    ParameterState<float> gainDb;
    ParameterState<bool> bypass;
};

struct EqualizerState
{
    ParameterState<bool> bypass;
};

struct DetectorState
{
    std::array<FilterState, 2> filters;
};

struct SaturatorState
{
    ParameterState<float> drive;
    ParameterState<float> outputDb;
    ParameterState<float> mix;
    ParameterState<float> detectorAmount;
    ParameterState<bool> bypass;
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
    ParameterState<bool> bypass;
    DetectorState detector;
};

} // namespace consolidator::dsp
