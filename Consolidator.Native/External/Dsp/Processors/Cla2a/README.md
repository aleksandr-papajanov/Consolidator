# Cla2aProcessor

[Native algorithm](Cla2aProcessor.cpp), [configuration](Cla2aProcessor.h),
[VST3 adapter](../../../../../ModelPlugins.Vst3/Source/Cla2aPluginProcessor.cpp).

The interface exposes Reduction, Output and Bypass. Reduction is normalized 0..1;
Output is linear amplitude 0..1, displayed -infinity..0 dB, with prepared +39 dB
calibration. Reduction changes threshold, not the audio input level. It specifies
sensitivity rather than guaranteed attenuation for every signal. Zero Reduction
stops new compression while optical memories recover; zero Output mutes.

Defaults: Reduction 0.598 and Output 0.02985085 (display -30.50 dB). These retain
the identified startup near reference Gain 40 / Peak Reduction 46. Reduction uses
the former reference-normalized Peak Reduction domain; its UI now shows 0..1.
The old Gain taper is replaced by nominal Output gain. Compressor mode and all
other reference settings remain as used during identification. Parameter IDs are
reduction/output/bypass. Use a fresh instance; old presets are not migrated.

This is a separate Native algorithm in consolidator::max::dsp::processors.
VST3 compiles the same source and owns only two host controls, Bypass, state and
preallocated float/double conversion. It does not override model coefficients.
CLA-2A is included in the Native project but is not connected to the runtime graph/ABI.

## Current algorithm

CLA-2A retains its identified optical response and variable threshold law.
Reduction passes through the measured two-region reference taper (pivot 0.4,
exponents 5.994/4.6213) to displayed reference units. Threshold is -2.70 dB at
reference position 40, with the measured six regional slopes above 40 and 1.69 dB
per unit below 40. The curve is cached until Reduction changes. This restores the
previous Peak Reduction sensitivity exactly; no fixed-threshold input calibration
or detector input multiplier remains.

The linked detector receives raw channel amplitudes and uses 0.1 ms charge /
0.3 ms recovery. The soft knee is 6.81 dB with reduction slope 0.768. Above 25 dB
reduction, subtract 0.135*excess^2/(excess+3), excess=max(reduction-25,0).
The preceding detector state determines target; this sample advances the detector.

Two dB memories track optical charge and recovery. Base charge drive is
sqrt((target+0.1)/(target+22)). Fast charge multiplies this by
(1+target/14)/(1+(target/17.7)^2), reproducing the stronger middle-depth attack
and slower deep-reduction attack. Fast/slow charge scales are 3.83/51.3 ms.

Recovery uses drive=1+target/4.33. The fast rate is
(1+fastMemory/10)*drive/0.308; the slow rate is
(1+slowMemory/10)*drive^2/3.35. At low stored reduction and zero target, their
limiting time constants are 0.308 and 3.35 seconds. The squared remaining-target
drive lets the slow memory recover quickly under continued compression while
retaining its long quiet tail. Backward Euler keeps each state between its previous
value and target. Slow fraction is 0.1903; their dB blend becomes audio gain.
Memory coefficients use one square root and rational arithmetic; the final
dB-to-gain conversion remains exponential.

Output applies its amplitude ramp times the prepared +39 dB calibration.
Displayed -39 dB gives unity output gain; -infinity mutes.

Audio colour consists of a first-order 4.3 Hz high pass and a −0.6 dB high shelf
at 6 kHz. State remains per channel; compression is linked. Preparation, curve
evaluation, detector update, optical gain and colour are separate methods. Config
is static constexpr; controls ramp over 10 ms. Process performs no allocation,
locking, I/O or host messaging. No added harmonic branch is present at this stage.

## Manual research

The primary grid contains 19 Gain/Peak Reduction combinations at 44.1 kHz, block 128;
12 additional combinations cover intermediate knobs and the instantiated defaults.
The 103-second raw probe combines four long step histories, seven stationary levels
and quiet tones from 8 Hz to 20 kHz. Results are local to EmulationLab/Experiments/cla2a;
raw input is preserved in EmulationLab/RawAudio/Probes/cla2a-level-timing-colour.wav.
A denser 46-condition grid uses displayed knob values and a 36-second probe:
three repeated 1/2/1-second −20/0/−20 dBFS cycles plus six 4-second level holds.
This separates cold startup, repeat-cycle memory and settled gain. Raw input:
EmulationLab/RawAudio/Probes/cla2a-short-repeated-9986.wav. Every render records
the actual parameter text and normalized values in a `.parameters.json` sidecar.
Reference default normalized values include Comp/Limiter 0, Hi Freq 0.5, Analog 1,
Mix 1 and Trim 0.5. [Findings and remaining limits](Findings.md).
[Research workflow](../../../../../Docs/Dsp/EmulationLab.md).
[DSP rules](../../../../../Docs/Dsp/Rules.md) prohibit automated audio tests.

## Interface change

Output = 10^((formerGainDb-39)/20). Reduction retains the former reference
normalized Peak Reduction value. The two optical memories, detector, knee, deep
curve and colour are unchanged. Automation of Reduction again moves the threshold
instead of scaling detector input. Earlier Input/Output conversion measurements
are historical; they do not describe the current control API.

## Manual verification of Reduction/Output

Native Release|x64 and this model's Release VST3 were built and installed.
A requested EmulationLab comparison used 46 static mapped settings on the
36-second repeated 998.6 Hz probe, at 44.1 kHz / block 128. Whole stereo waveforms
were compared with saved preceding-model renders, including attacks, releases
and quiet recovery. Maximum relative waveform RMS difference was
4.16424e-7 (0.0000416%); maximum sample difference
relative to the preceding waveform peak was 4.70678e-7.
These observations cover mapped static settings, not equivalence during automation
or an accuracy bound against Waves. Optical/timing and colour constants are retained.
No DSP automated tests were created or run. Source review found no new allocation,
locking, I/O, host-side algorithm, duplicate API or preset migration path.
