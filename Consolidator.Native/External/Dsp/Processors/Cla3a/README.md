# Cla3aProcessor

[Native implementation](Cla3aProcessor.cpp),
[constants](Cla3aProcessor.h),
[VST3 adapter](../../../../../ModelPlugins.Vst3/Source/Cla3aPluginProcessor.cpp).

## Scope and ownership

A simplified optical compressor based on Waves CLA-3A Stereo in Compressor mode.
One Native implementation is compiled by Native and VST3; the adapter owns only
host parameters/state, Bypass and preallocated float/double conversion buffers.
The processor is not connected to the Managed runtime graph/ABI.

| Control | Range | Default |
| --- | --- | --- |
| Reduction | 0..1 | 0.49237375 |
| Output | amplitude 0..1, displayed -infinity..0 dB | 0.12 (-18.42 dB displayed) |

Reduction changes threshold without driving audio input. Zero stops new reduction;
existing optical memories recover. Output remains independent, with a prepared
+30 dB calibration; displayed -30 dB is unity and -infinity mutes. Controls ramp
in 10 ms. Native owns defaults and configuration. Host IDs are
reduction/output/bypass; old presets are not migrated.

## Detector and curve

Each selected input channel receives a first-order detector shelf: unity HF gain,
-1.8 dB LF gain, corner 740 Hz. With low=sample-highpass(sample), detector sample
is sample-(1-lowGain)*low. Link by maximum absolute amplitude. One power follower
charges in 0.1 ms and discharges in 1.7 ms; its preceding state controls current gain.
The detector receives the weighted raw amplitude without an Input multiplier.
Reduction moves threshold linearly: T=18.3-60*Reduction dB. At the default,
T is about -11.2424 dB, retaining the previous startup sensitivity. Maximum
Reduction reaches -41.7 dB, equivalent to the preceding maximum detector gain.
Slope remains 0.735 and knee 12.5 dB. Threshold and lower-knee power are cached
until Reduction changes. Reduction is sensitivity, not a fixed requested number
of dB of attenuation. Its prior reference knob polynomial did not survive in
committed history; this simple taper restores the variable-threshold scheme,
not the exact former Waves knob positions.

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
DspBiquadCascade. A prepared +30 dB output calibration retains maximum audio gain. Displayed Output 0 dB is maximum gain, and -infinity mutes.
Effective unity is displayed Output -30 dB. Calibration converts once in Prepare;
Process multiplies it by the current Output amplitude ramp.

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

## Mapping from the preceding Input/Output model

For old input amplitude I>0:

~~~text
Reduction = (20*log10(I)+60)/60
Output = old Output amplitude
~~~

This preserves the static detector over-threshold level, knee and optical memories
for I in 0.001..1. Old Input zero maps to Reduction zero. Weaker sensitivities below
0.001 are intentionally replaced by the no-new-compression endpoint. Floating-point
rounding prevents bitwise identity. Automation now moves threshold directly.

## Code map

- PrepareDetector and PrepareColourFilters compile fixed coefficients before processing.
- UpdateReductionCurve caches the variable threshold and knee start.
- CalculateTargetReductionDb reads the preceding detector power at this threshold.
- UpdateDetector applies detector weighting, then advances linked power.
- UpdateOpticalGain advances fast/slow dB memories and returns their combined gain.
- ProcessHighPassSample runs the audio HP; the shelf and Output retain their separate block passes.

The optical model is complete. This interface change retains optical/filter coefficients and state update order.

## Manual verification of Reduction/Output

Native Release|x64 and this model's Release VST3 were built and installed.
A requested EmulationLab comparison used 3 static mapped settings on the
36-second repeated 998.6 Hz probe, at 44.1 kHz / block 128. Whole stereo waveforms
were compared with saved preceding-model renders, including attacks, releases
and quiet recovery. Maximum relative waveform RMS difference was
3.12944e-7 (0.0000313%); maximum sample difference
relative to the preceding waveform peak was 3.14234e-7.
These observations cover mapped static settings, not equivalence during automation
or an accuracy bound against Waves. Optical/timing and colour constants are retained.
No DSP automated tests were created or run. Source review found no new allocation,
locking, I/O, host-side algorithm, duplicate API or preset migration path.
