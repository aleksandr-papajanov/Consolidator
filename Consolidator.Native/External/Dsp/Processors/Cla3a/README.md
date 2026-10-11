# Cla3aProcessor

[Native implementation](Cla3aProcessor.cpp),
[constants](Cla3aProcessor.h),
[VST3 adapter](../../../../../ModelPlugins.Vst3/Source/Cla3aPluginProcessor.cpp).

## Scope and ownership

A simplified optical compressor based on manual comparison with Waves CLA-3A
Stereo in Compressor mode. Input/Output use amplitude controls 0..1 with the same
-infinity..0 dB display and 0.3 host-range skew as CLA-76. Defaults display
about -30.46/-18.42 dB, from Native amplitude defaults 0.03/0.12. Both use 10 ms amplitude
ramps. Other reference settings retain instantiated defaults.
Exact hardware/plugin matching is not a goal; small differences are accepted for
runtime simplicity. One Native implementation is compiled directly by VST3.
The adapter owns only host parameters/state, Bypass and preallocated conversion.
The processor is included in the Native project and compiled directly into VST3;
it is not yet connected to the Managed runtime graph/ABI. No extra research controls.

| Control | Host display | Default amplitude | Effective default gain |
| --- | --- | --- | --- |
| Input | -infinity..0 dB | 0.03 | detector about -14.46 dB |
| Output | -infinity..0 dB | 0.12 | audio about +11.58 dB |

Native Cla3aControls owns the defaults; Cla3aConfig owns calibration and algorithm
constants. VST3 owns only the display conversion and host parameter mapping.

## Detector and curve

Each selected input channel receives a first-order detector shelf: unity HF gain,
-1.8 dB LF gain, corner 740 Hz. With low=sample-highpass(sample), detector sample
is sample-(1-lowGain)*low. Link by maximum absolute amplitude. One power follower
charges in 0.1 ms and discharges in 1.7 ms; its preceding state controls current gain.
Input scales the linked detector amplitude before the power follower and does not
amplify the audio path. A prepared +16 dB calibration preserves the previous
maximum detector sensitivity. Displayed Input 0 dB applies this maximum; its
zero-amplitude endpoint feeds zero power while existing histories decay.

Threshold is fixed at -25.7 dB, matching the normal CLA-76 model threshold.
Slope is 0.735; knee is 12.5 dB. Lower-knee power prepares once.
The former threshold polynomial is removed from runtime. Input is a simple
amplitude multiplier; no sample gain conversion or control cache is needed.

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

Compression -> second-order highpass -> high shelf -> Output.
Highpass is 8.8 Hz/Q 0.62. High shelf is 14.5 kHz/Q 0.64/+2.2 dB, using the existing
DspBiquadCascade. A prepared +30 dB output calibration preserves the previous
maximum audio gain. Displayed Output 0 dB is maximum gain, and -infinity mutes.
Effective unity is displayed Output -30 dB. Calibration converts once in Prepare;
Process multiplies it by the current amplitude ramp.

Compression/highpass share a sample loop, followed by the shelf and Output loops.
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
[findings](Findings.md).

## Control conversion

For static controls, the former threshold T and Gain amount G map to:

~~~text
InputDisplayDb = -25.7 - T - 16
OutputDisplayDb = 50*(G - 0.408) - 30
~~~

Scaling detector power adds InputDisplayDb+16 to its measured dB level, preserving the
over-threshold value, knee target and optical histories from a cold start.
Floating-point rounding prevents a bitwise guarantee. Automation transitions
are intentionally different. Host parameter IDs are now input/output/bypass;
use a fresh instance instead of an old Gain/Peak Reduction preset.

Manual EmulationLab comparison against the approved simplified model used 15
mapped static control combinations at 44.1 kHz, block size 128, on the continuous
56-second four-level-pair 998.6 Hz probe. Maximum relative whole-waveform RMS
error was 4.623e-7 (0.000047%), including attack/release transitions. This measures
control conversion against the previous model, not accuracy against Waves.
The attenuation interface retains the previous maximum gains and extends both
lower endpoints to zero amplitude. Zero Input removes detector excitation;
zero Output mutes the audio.
Use a fresh instance after this range/unit change; preset migration is not provided.

## Code map

- PrepareDetector and PrepareColourFilters compile fixed coefficients before processing.
- CalculateTargetReductionDb reads the preceding detector power at the fixed threshold.
- UpdateDetector applies detector weighting and sensitivity, then advances linked power.
- UpdateOpticalGain advances fast/slow dB memories and returns their combined gain.
- ProcessHighPassSample runs the audio HP; the shelf and Output retain their separate block passes.

The model is complete. This organization refactor changes source layout and method boundaries;
control ranges, constants, state update order and signal routing are retained.
