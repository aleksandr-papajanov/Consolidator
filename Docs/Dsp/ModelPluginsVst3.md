# Consolidator model VST3 plugins

ModelPlugins.Vst3 builds thin JUCE adapters over production Native DSP source.
The five finished models compile their corresponding .cpp files from
`Consolidator.Native/External/Dsp/Processors/<model>`. Code and algorithm documents
share the model folder; C++ namespace is `consolidator::max::dsp::processors`.
No adapter maintains an alternate algorithm.
The adapters are hosted independently by Plugin Doctor and EmulationLab.

## Thick and Air

Consolidator Thick Model exposes Thick and Bypass.
Consolidator Air Model exposes Air and Bypass. Coefficient overrides are removed;
immutable coefficients belong to Native headers. Each has its own
VST3 class ID, state and parameter manifest. These represent separate Polish
processing boundaries. See [Thick](../../Consolidator.Native/External/Dsp/Processors/Thick/README.md) and [Air](../../Consolidator.Native/External/Dsp/Processors/Air/README.md).

## Transient Shaper

Consolidator Transient Shaper Model calls the shared Native
[TransientShaperProcessor](../../Consolidator.Native/External/Dsp/Processors/TransientShaper/README.md). CMake compiles exactly
that .cpp and DspParameterRamp.cpp; the Native project already includes the same
processor. Detectors, masks, signed gain law, HP10, output gain smoothing and both
permanent clamp stages belong to that processor. There is no model-only DSP file,
Limit control or disabled-Limit path.

Attack and Sustain use −1…+1, neutral 0. Output Gain defaults to 0 dB, range −12…+12.
The adapter owns host parameters/state, raw Bypass and preallocated stereo
float/double conversion buffers. Process applies live amounts/output gain, invokes
Native Process once and converts the result back to float. It contains no audio
filter or limiter math. Existing parameter IDs and VST3 class ID are retained.

Research configuration controls are removed. Native and VST3 use the same immutable
TransientShaperConfig constants. Presets restore only current user controls after
checking their presence and finite values, without loading obsolete configuration nodes.

## Research workflow

Generate parameter descriptions with EmulationLab plugin parameter-manifest.
Amounts belong to condition controls. Finished models do not expose optimizer
parameters. Their fixed configuration is compiled into the Native implementation.
For a Transient reference with a Limit switch, keep it enabled; the candidate has
no Limit binding. Current local comparisons use saved previous-model renders.
See [EmulationLab](EmulationLab.md) for commands and interpretation.

A new emulation target adds a plugin target and thin adapter here, compiling its
production Native processor. Algorithms must not be copied into adapter source.

## CLA-76 Bluey

Consolidator CLA-76 Bluey Model compiles the shared [Cla76Processor](../../Consolidator.Native/External/Dsp/Processors/Cla76/README.md).
It contains five user controls: Input, Output, Attack, Release and Ratio, plus Bypass.
Attack/Release/Ratio are numeric 0..1. Attack/Release increase with time; Ratio
selects 4:1 at zero and ALL at one. Input/Output use nominal amplitude gain with
dB text display. Defaults come from Native Cla76Controls.

There are no research controls or coefficient overrides. Immutable Native configuration
owns all detector, compression, timing, memory and filter coefficients. The adapter
owns host state, generic UI, Bypass and preallocated stereo float/double conversion;
its dB text conversion is UI formatting, not another audio algorithm.

Presets contain the six host controls. All current controls must be valid before
restoration; no aliases or migration exist. Processor Reset preserves control
targets and clears histories. The Native graph has not been extended for CLA-76.
Research is complete; useful observations are in [findings](../../Consolidator.Native/External/Dsp/Processors/Cla76/Findings.md).

## CLA-3A

Consolidator CLA-3A Model compiles the shared [Cla3aProcessor](../../Consolidator.Native/External/Dsp/Processors/Cla3a/README.md).
Input/Output use amplitude 0..1 with -infinity..0 dB text and the same 0.3 skew
as CLA-76. Native amplitude defaults 0.03/0.12 display about -30.46/-18.42 dB. Prepared Native calibration
adds +16/+30 dB respectively, retaining the established maximum gains.
Bypass and these two controls are the complete host/preset interface. Input changes only detector
sensitivity at a fixed -25.7 dB threshold; Output changes audio gain.
Use fresh instances: current parameter IDs are input/output/bypass, with no migration.
Reference Compressor mode and all remaining default settings are fixed.
The adapter contains no DSP algorithm, configuration overrides or independent constants.
The optical model has manually fitted stationary gain and target-dependent release.
Timing, simplified fixed colour and current figures are in [Cla3aProcessor](../../Consolidator.Native/External/Dsp/Processors/Cla3a/README.md).
Research conclusions are in [Cla3aFindings](../../Consolidator.Native/External/Dsp/Processors/Cla3a/Findings.md).
The fixed detector weighting also lives in Native. Harmonic matching and broader
dynamic coverage remain open; the adapter has no filter code.
