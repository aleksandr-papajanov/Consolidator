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

## CLA-76 Bluey

Consolidator CLA-76 Bluey Model compiles the shared [Cla76Processor](Cla76Processor.md).
It contains five user controls: Input, Output, Attack, Release and Ratio, plus Bypass.
Attack/Release/Ratio are numeric 0..1. Attack/Release increase with time; Ratio
selects 4:1 at zero and ALL at one. Input/Output use nominal amplitude gain with
dB text display. Defaults come from Native Cla76Controls.

There are no research controls or coefficient overrides. Immutable Native tuning
owns all detector, compression, timing, memory and filter coefficients. The adapter
owns host state, generic UI, Bypass and preallocated stereo float/double conversion;
its dB text conversion is UI formatting, not another audio algorithm.

Presets contain the six host controls. All current controls must be valid before
restoration; no aliases or migration exist. Processor Reset preserves control
targets and clears histories. The Native graph has not been extended for CLA-76.
Research is complete; useful observations are in [findings](Cla76Findings.md).

## CLA-3A

Consolidator CLA-3A Model compiles the shared [Cla3aProcessor](Cla3aProcessor.md).
Input/Output use amplitude 0..1 with -infinity..0 dB text and the same 0.3 skew
as CLA-76. Native defaults display -30.04625/-22.9 dB. Prepared Native calibration
adds +16/+30 dB respectively, preserving previous maximum gains and default sound.
Bypass and these two controls are the complete host/preset interface. Input changes only detector
sensitivity at a fixed -25.7 dB threshold; Output changes audio gain.
Use fresh instances: current parameter IDs are input/output/bypass, with no migration.
Reference Compressor mode and all remaining default settings are fixed.
The adapter contains no DSP algorithm, tuning overrides or independent constants.
The optical model has manually fitted stationary gain and target-dependent release.
Timing, simplified fixed colour and current figures are in [Cla3aProcessor](Cla3aProcessor.md).
Research conclusions are in [Cla3aFindings](Cla3aFindings.md).
The fixed detector weighting also lives in Native. Harmonic matching and broader
dynamic coverage remain open; the adapter has no filter code.
