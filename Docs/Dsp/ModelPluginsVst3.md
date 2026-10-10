# Consolidator model VST3 plugins

ModelPlugins.Vst3 builds thin JUCE adapters over production Native DSP source.
Thick, Air and Transient Shaper compile their corresponding processor .cpp files
from Consolidator.Native/External/Dsp. No adapter maintains an alternate algorithm.
The adapters are hosted independently by Plugin Doctor and EmulationLab.

## Thick and Air

Consolidator Thick Model exposes Thick, Bypass and PolishThickTuning.
Consolidator Air Model exposes Air, Bypass and PolishAirTuning. Each has its own
VST3 class ID, state and parameter manifest. These represent separate Polish
processing boundaries. See [Thick](PolishThickProcessor.md) and [Air](PolishAirProcessor.md).

## Transient Shaper

Consolidator Transient Shaper Model calls the shared Native
[TransientShaperProcessor](TransientShaperProcessor.md). CMake compiles exactly
that .cpp and DspParameterRamp.cpp; the Native project already includes the same
processor. Detectors, masks, signed gain law, HP10, output gain smoothing and both
permanent clamp stages belong to that processor. There is no model-only DSP file,
Limit control or disabled-Limit path.

Attack and Sustain use −1…+1, neutral 0. Output Gain defaults to 0 dB, range −12…+12.
The adapter owns host parameters/state, raw Bypass and preallocated stereo
float/double conversion buffers. Process applies live amounts/output gain, invokes
Native Process once and converts the result back to float. It contains no audio
filter or limiter math. Existing parameter IDs and VST3 class ID are retained.

The 21 tuning controls come from TransientShaperTuning defaults and are sampled
only in prepareToPlay. EmulationLab applies a complete candidate and reprepares
the plugin before rendering. Native uses header defaults; a saved VST3 tuning
state can override them. Such overrides do not create another implementation.

## Research workflow

Generate parameter descriptions with EmulationLab plugin parameter-manifest.
Amounts belong to condition controls; tuning coefficients belong to fitting stages.
Current Transient manifests set reference Limit1 and contain no candidate Limit binding.
See [EmulationLab](EmulationLab.md) for commands and interpretation.

A new emulation target adds a plugin target and thin adapter here, compiling its
production Native processor. Algorithms must not be copied into adapter source.
