# SslChannelProcessor

[Native core](SslChannelProcessor.cpp), [configuration](SslChannelProcessor.h),
[VST3 adapter](../../../../../ModelPlugins.Vst3/Source/SslChannelPluginProcessor.cpp),
[research conclusions and limits](Findings.md).

## Scope and ownership

The accepted simplified Waves SSLChannel Stereo compressor section: Ratio,
Release, Attack mode and Threshold. Gate, EQ, filters and channel routing are
outside the processor. One Native implementation is compiled by Native and VST3,
in namespace consolidator::max::dsp::processors. It is not connected to the
Managed runtime graph/ABI. VST3 owns only host parameters/state, bypass and
preallocated float/double conversion; there is no host-side DSP implementation.

| Control | Native units / host display | Default |
| --- | --- | --- |
| Ratio | 1..100, limiting endpoint displayed Inf | 1 |
| Release | 0.1..4 seconds | 0.4 s |
| Fast Attack | Off/On | Off |
| Threshold | -20..+10 dBu | 0 |

Defaults come from the reference Full Reset instance: Analog Off, expander range
zero, EQ gains zero, filters Out, input/output gain zero, dynamics active. Only
these four compressor controls were changed during identification. Waves prints
CompRelease with an ms suffix; the model follows the documented seconds range.
Host IDs are ratio/release/fastAttack/threshold/bypass. Numeric parameters ramp
for 10 ms; Fast Attack is a switch. Configuration is immutable Native constexpr.

## Effective algorithm

Linked maximum-channel power feeds a 1 ms RMS follower. The preceding power state
controls this sample's gain. The positive rational compression onset is:

~~~text
x = max(detectorLevelDb - detectorOnsetDb, 0)
targetDb = slope * x*x/(x + bendDb)
~~~

Slope/onset/bend are continuous low-order functions of inverse Ratio, Threshold,
log Release and Attack mode. Coefficient arrays are function coefficients, not
interpolation anchors. Above 20:1 the finite-ratio function morphs smoothly into
limiting. Effective onset is calibrated from measured dBFS versus reference dBu;
the fixed RMS-to-peak offset is folded into the cached detector onset. Samples
below cached onset power skip log10. Ratio 1 has zero compression and makeup.

Automatic makeup uses an inverse-Ratio polynomial and Threshold law with an
additional slope below -10 dBu. It only recomputes when Ratio/Threshold change.
Two dB memories charge toward target and recover toward zero, stopping at the
current signal's reduction floor. Their blend becomes audio attenuation. Normal
and Fast charging use prepared coefficients; normal program-sensitive attack is
approximated, not an exact identified auto-attack circuit. Release laws are
positive power functions with time floors and mode-specific coefficients.

## Preparation and realtime path

- Prepare compiles detector and Attack coefficients and prepares ramps.
- UpdateControlCoefficients caches slope/onset/bend when live controls change.
- UpdateMakeup compiles compensation only for Ratio/Threshold changes.
- UpdateTiming compiles retention factors only for Release/Attack changes.
- CalculateTargetReductionDb reads the preceding detector power.
- UpdateReductionMemory advances bounded charge/recovery histories.
- Process links channels and applies compression/makeup in one audio loop.

Recovery stores retention directly, avoiding per-sample subtraction. Audio gain
uses exp with a fixed dB-to-log factor rather than general pow; zero reduction
skips the exponential. The zero Fast coefficient is removed from the slope law.
Meaningful measured curve/timing coefficients are retained. There is no added
saturation, noise, oversampling, gate, EQ, parallel audio path or limiter branch.
Process contains no allocation, lock, I/O, logging, fitting or Managed call.
CPU improvement was not benchmarked; the changes remove identifiable operations
and redundant coefficient work without claiming a percentage speedup.

Native Release|x64 and the Release VST3 are built and installed. DSP automated
suites are prohibited by [rules](../../../../../Docs/Dsp/Rules.md). Raw probes
remain in EmulationLab/RawAudio; temporary experiment outputs are removed after
research. The accepted model retains documented reference deviations; this is
not a claim of exact waveform emulation.
