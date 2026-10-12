# Consolidator model VST3 plugins

ModelPlugins.Vst3 builds thin JUCE adapters over production Native DSP source.
The five finished models compile their corresponding .cpp files from
`Consolidator.Native/External/Dsp/Processors/<model>`. Code and algorithm documents
share the model folder; C++ namespace is `consolidator::max::dsp::processors`.
No adapter maintains an alternate algorithm.
The adapters are hosted independently by Plugin Doctor and EmulationLab.

CLA-2A is a standalone simplified model: [algorithm and scope](../../Consolidator.Native/External/Dsp/Processors/Cla2a/README.md).
Its adapter exposes Reduction 0..1, nominal amplitude Output displayed
-infinity..0 dB, and Bypass. Native defaults are Reduction 0.598 and Output
0.02985085 (-30.50 dB displayed), with +39 dB output calibration. Reduction restores
the identified Peak Reduction threshold law; the former Gain taper is removed.
Optical memories and colour are retained. Use fresh instances: IDs are
reduction/output/bypass, without preset migration. See the model README for
historical reference control mapping and measured limits.

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
It contains Input, Output, Attack, Release and Ratio, plus Bypass. Input/Output
restore the fitted reference attenuator functions in Native, with -inf..0 dB UI
text. Defaults are -30/-18 dB. Attack/Release use reference 1..7, slow to fast,
defaults 3/4. Ratio retains 0..1 morphing from 4:1 to ALL. Old nominal-gain and
Reduction interfaces are removed. Timing functions and simplified colour remain.

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
Gain/Peak Reduction restore their reference-normalized 0..1 control laws in Native;
the host displays/accepts reference 0..10 units. Defaults are 5.50/4.00,
normalized 0.55/0.4. The final
simplified cubic threshold and linear-in-dB Gain law are recovered from the saved
chat. Current detector/optical/filter optimizations remain. Host IDs are
gain/peakReduction/bypass; use fresh instances, with no preset migration.
Reference Compressor mode and all remaining default settings are fixed.
The adapter contains no DSP algorithm, configuration overrides or independent constants.
The optical model has manually fitted stationary gain and target-dependent release.
Timing, simplified fixed colour and current figures are in [Cla3aProcessor](../../Consolidator.Native/External/Dsp/Processors/Cla3a/README.md).
Research conclusions are in [Cla3aFindings](../../Consolidator.Native/External/Dsp/Processors/Cla3a/Findings.md).
The fixed detector weighting also lives in Native. Harmonic matching and broader
dynamic coverage remain open; the adapter has no filter code.

## SSLChannel compressor

[Shared Native model](../../Consolidator.Native/External/Dsp/Processors/SslChannel/README.md),
[findings](../../Consolidator.Native/External/Dsp/Processors/SslChannel/Findings.md).
ConsolidatorSslChannelModel_VST3 compiles SslChannelProcessor directly from Native.
Controls are Ratio 1..Inf, Release 0.1..4 seconds, Fast Attack switch and Threshold
-20..+10 dBu, plus raw Bypass. Default reference settings are used outside this
section. The adapter owns only parameter mapping/state, UI and conversion buffers;
Native owns all coefficients, ramps, detector/memories and automatic makeup.
The simplified model is accepted; documented reference deviations are retained.
