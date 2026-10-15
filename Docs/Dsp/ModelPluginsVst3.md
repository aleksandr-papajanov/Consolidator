# Consolidator model VST3 plugins

ModelPlugins.Vst3 builds thin JUCE adapters over production Native DSP source.
The four VST3 targets compile their corresponding .cpp files from
`Consolidator.Native/External/Dsp/Processors/<model>`. Code and algorithm documents
share the model folder; C++ namespace is `consolidator::max::dsp::processors`.
No adapter maintains an alternate algorithm.
The adapters are hosted independently by Plugin Doctor and EmulationLab.

The universal compressor uses Normal mode by default: Reduction 0..1, nominal Output
-24..+24 dB, Bypass, and Attack/Release/Ratio 0..1 where available. Defaults are
Reduction 0.5, Output 0 dB. All time/ratio directions run from smaller to larger.
Emulation is retained only in shared Native source; no current adapter activates
it or exposes a mode switch. [Control modes and mappings](CompressorControlModes.md).

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

## Universal compressor

[DeviceCompressor](../../Consolidator.Native/External/Dsp/Processors/DeviceCompressor/README.md)
selects the shared CLA-76, CLA-3A, CLA-2A or SSL Native algorithm. The only compressor
target is ConsolidatorCompressor_VST3, installed as Consolidator Compressor.vst3.
The four individual compressor adapters/targets are removed. Thick, Air and
Transient Shaper remain separate targets.

Controls in order: Algorithm (four-step slider), Reduction, Output, Attack, Release,
Ratio, Auto Gain (default ON), Bypass. Reduction/timing/ratio use 0..1, Output -24..+24 dB.
CLA-2A/3A disable Attack/Release/Ratio in the editor and ignore them in Native.
SSL Attack snaps to two endpoints in both UI and Native; all four use musical-source makeup in Normal;
SSL Emulation alone retains its reference makeup law. Only the active model processes audio. Switching resets
that model before the next block, without allocation/reprepare/crossfade.

State stores the eight current controls; old-plugin presets are not migrated.
Auto Gain controls actual Output and publishes it to the host on the message thread.
Manual Output changes disable Auto Gain; unchecking it holds the last audio snapshot.
No second hidden adaptive gain is applied. [Lifecycle and measurement](AdaptiveCompressorGain.md).
Custom UI polls Algorithm on the message thread for host automation/preset changes.
The shared [control-mode contract](CompressorControlModes.md) describes Native
reference modes retained for future use. The current adapter uses Normal only.

Normal Ratio starts at 1:1 in both selectable 76 and SSL; their shared default is
0.4 (4:1), using a shared rational taper. The 76 upper section retains the existing 4:1-to-ALL morph, relocated
to this shared coordinate. Reference Emulation defaults are unchanged.
