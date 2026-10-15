# Cla3aProcessor

[Native implementation](../../Audio/Algorithms/Cla3a/Cla3aProcessor.cpp),
[constants](../../Audio/Algorithms/Cla3a/Cla3aProcessor.h),
[VST3 adapter](../../../ModelPlugins.Vst3/Source/DeviceCompressorPluginProcessor.cpp).

## Текущее управление

По умолчанию используется Normal: Reduction 0…1 (default 0.5), номинальный
Output −24…+24 дБ (default 0 дБ). Reduction линейно меняет порог; Output
напрямую масштабирует выход; Normal-компенсация подобрана отдельно по музыкальным источникам.
[Полный контракт режимов](CompressorControlModes.md).
Emulation остаётся только в Native-коде и выбирается конструктором.
Ниже референсные шкалы и измерения описывают этот режим, не единый VST3 UI.

## Emulation: scope and ownership

A simplified optical compressor based on Waves CLA-3A Stereo in Compressor mode.
One Native implementation is compiled by Native and VST3; the adapter owns only
host parameters/state, Bypass and preallocated float/double conversion buffers.
The main strip selects this Normal model through CompressorDevice/SnapshotBinding.
The research adapter compiles this same Native implementation.

| Control | Native domain | Default |
| --- | --- | --- |
| Gain | normalized 0..1, displayed reference 0..10 | 0.55 (5.50) |
| Peak Reduction | normalized 0..1, displayed reference 0..10 | 0.4 (4.00) |

The reference control contract is restored from the saved conversation's final
simplified model, before Input/Output and Reduction/Output conversions. Gain is
not nominal dB: outputGainDb=50*(Gain-0.408). Gain zero is finite attenuation,
not mute. Peak Reduction zero stops new compression while optical memories
recover. Both controls ramp 10 ms and are cached. Native owns defaults and
control laws; Emulation uses these reference controls; current VST3 exposes Reduction/Output plus Bypass.
Current host IDs are reduction/output/bypass. Use a fresh instance; obsolete presets
are not migrated. Current optical/filter coefficients and optimized methods
remain unchanged.

## Detector and curve

Each selected input channel receives a first-order detector shelf: unity HF gain,
-1.8 dB LF gain, corner 740 Hz. With low=sample-highpass(sample), detector sample
is sample-(1-lowGain)*low. Link by maximum absolute amplitude. One power follower
charges in 0.1 ms and discharges in 1.7 ms; its preceding state controls current gain.
The detector receives the weighted raw amplitude without an Input multiplier.
Peak Reduction restores the fitted cubic threshold law:

~~~text
u = (PeakReduction - 0.6)/0.4
T = -27.37 + u*(-22.94 + u*(14.42 - 5.13*u))
~~~

Slope is 0.735 and knee 12.5 dB. The lower-knee power and output gain are cached
until their respective controls change. Emulation has no input sensitivity multiplier; Normal uses a separate linear Reduction taper. The polynomial was missing from committed
history but has now been recovered from the saved chat's final simplified snapshot.

## Optical histories

Two dB reduction states charge in 5/75 ms. Slow fraction is 0.18. For curve target g,
each discharging state's release time and update weight are:

~~~text
seconds = 0.00006 + baseSeconds*exp(-g/5.8)
coefficient = 1/(1 + sampleRate*seconds)
state += coefficient*(target-state)
~~~

Fast/slow bases are 0.105/0.8 seconds. Strong remaining compression shortens
release; target zero keeps a slow tail. Backward Euler keeps the release weight
stable for positive time constants and removes two sample exponentials. Charging
weights use the prepared TimeCoefficient primitive. Zero-target release weights
prepare once; positive-target release shares one exponential. No event timer,
coupled-release path or additional detector.

## Audio path and runtime

Emulation audio path: Compression -> second-order highpass -> high shelf -> Output.
Highpass is 8.8 Hz/Q 0.62. High shelf is 14.5 kHz/Q 0.64/+2.2 dB, using the existing
BiquadCascade. Gain uses the reference law above; the temporary +30 dB calibration and
nominal amplitude Output control are removed.
The high-pass coefficients use the shared `DesignHighPass` filter primitive with
independent per-channel biquad histories. These output colour filters do not feed
the detector; Normal skips and does not prepare them.

Compression runs first, followed in Emulation by the high-pass and shelf passes;
Output gain runs last. Normal omits both colour filters and their preparation.
The extra highpass pole, two LF corrective bells, nonlinear bass reinforcement,
tiny colour offset and +0.063 dB detector HF correction were removed. No added
saturation, noise/hum or oversampling. Filter/state storage is fixed. Prepare
compiles coefficients and clears the shelf before configuring it so rate changes
recompile unchanged parameters. Reset clears histories and snaps existing ramps.
Process has no allocation, lock, I/O, logging, fitting or Managed call.

## Final manual comparison

Release target ConsolidatorCla3aModel_VST3 was built and installed.

| Metric | Before | Simplified |
| --- | --- | --- |
| Short-history attack shape RMSE, dB | 0.59578 | 0.59652 |
| Short-history normalized recovery RMSE | 0.043440 | 0.043623 |
| Short-history plateau RMSE, dB | 0.42710 | 0.42397 |
| Frequency/level gain RMSE, 144 points, dB | 0.29494 | 0.30240 |
| Quiet gain RMSE from 20 Hz, 60 points, dB | 0.10594 | 0.12637 |

Timing uses the separate 998.6 Hz four-pair 1/2/1-second input at 44.1 kHz. Attack
samples log-spaced 1 ms peaks at 1..200 ms relative to each output's high plateau;
recovery samples 10..800 ms normalized to each output's endpoints. Frequency grids
include 44.1/96 kHz. These are requested manual observations, not DSP test suites
or universal accuracy claims. One final approximate timing proposal improved by
less than 1% and was not retained. Rounded constants are intentional.

CPU speedup was not measured. Structurally, audio passes drop from six to three;
positive-target adaptive release uses one exponential instead of up to three.
Source review found no new ownership/realtime issues. Useful limitations:
[findings](Cla3aFindings.md).

## Code map

- PrepareDetector always compiles detector coefficients; PrepareColourFilters runs only in Emulation.
- UpdateReductionCurve caches the variable threshold and knee start.
- CalculateTargetReductionDb reads the preceding detector power at this threshold.
- UpdateDetector applies detector weighting, then advances linked power.
- UpdateOpticalGain advances fast/slow dB memories and returns their combined gain.
- Emulation applies the audio high-pass, shelf and Output in separate passes; Normal skips the colour passes.

The restored interface retains the final simplified optical/filter coefficients and state update order.

## Restoration scope

Recovered control coefficients are already the rounded, simplified final values,
not the early identification prototype. Existing two optical histories, adaptive
release, shelf, highpass, caches and realtime processing remain. Historical
simplification figures above describe those algorithm decisions; this restoration
is not a new claim of exact matching to Waves on all inputs. Native/VST3 compile
the same processor. Live reference metadata (1001 positions per knob) confirms both readouts are
linear 0..10. The VST3 formats/accepts that same scale, Native retains normalized
0..1. The display conversion contains no audio algorithm.

## Universal VST3

The individual adapter/target is removed. This Native model is now selected by
[DeviceCompressor](DeviceCompressor.md) in Consolidator Compressor.
Its UI owns the shared Algorithm/Reduction/Output/Attack/Release/Ratio controls.

## Normal automatic makeup

Normal uses a compact settings law fitted to eight unnormalised musical sources,
with equal source weights. It balances average loudness across algorithms rather
than preserving a particular sine operating point. Optical models depend only on
Reduction; 76/SSL also include Attack, Release and Ratio. No running loudness
matcher or new detector history is added. Original Emulation gain is unchanged.
[Method, results and limits](ProgramMakeup.md).
