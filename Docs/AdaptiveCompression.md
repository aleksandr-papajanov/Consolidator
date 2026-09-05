# Adaptive Compression

## Controls and ownership

Native `CompressionProcessor` implements the existing Managed
`compressor.compression` amount (`0..1`) and `compressor.character` selector.
The ABI and state paths are unchanged. UI order remains `0 = Punch`,
`1 = Tight`, `2 = Smooth`; Character selects timing, not a second amount.
The processor owns its envelopes and controllers on the audio thread.

| Compression | Nominal target GR |
| --- | --- |
| 0% | 0 dB |
| 25% | 2 dB |
| 50% | 5 dB |
| 75% | 9 dB |
| 100% | 18 dB |

Targets interpolate linearly between these points. Ratio increases continuously
from 1:1 to 8:1. A 6 dB soft knee smooths the onset of reduction.

## Detection and automatic threshold

The compressor measures linked mean-square energy across the selected channels.
Its own detector EQ captures main audio before gain reduction. Transient shaping
runs after compression and its makeup, with a fresh detector capture before its own gain;
the two filter histories do not advance twice on the same filter instance.

A 2 ms energy follower supplies the fast level and a 500 ms follower supplies
the reference. The nominal threshold is
`referenceDb - targetGR / (1 - 1 / ratio)`. The implementation evaluates the
equivalent GR expression without division by a near-zero ratio slope.
Consequently an overall change in source gain moves the threshold with it.

A slow integral trim compares target GR with a 500 ms average of applied
reduction and adjusts the reduction command, bounded to +/-6 dB before amount
scaling. Its integration time is 3 seconds. This helps compensate for timing
and programme statistics; the knob does not guarantee a particular instantaneous
GR, nor exact target convergence for sparse impulses or arbitrary signals.
Learning stops below -90 dBFS detector RMS; silence releases GR without lowering
the reference indefinitely. Internal reduction is capped at 30 dB.

## Timing

| Mode | Gain attack | Programme-dependent release |
| --- | --- | --- |
| Punch | 25 ms | 60–120 ms |
| Tight | 1 ms | 45–90 ms |
| Smooth | 10 ms | 250–500 ms |

A 500 ms measure of sustained reduction above 1 dB interpolates the release
between its limits. Timing changes use the common 10 ms scalar ramp.
Attack figures describe the gain controller; the detector adds its own response.
There is no lookahead.

## Makeup and parallel

Separate 1 second energy averages measure actual main audio before and after
gain reduction, excluding makeup. The controller returns 75% of the measured
loss in dB, limited to +12 dB and smoothed over 1.5 seconds. It freezes learning
in silence. This is partial RMS compensation, not perceptual loudness matching.
At high Compression the output can still be quieter.

Transient shaping and its gentle autogain follow compression/makeup, then Output
applies. Parallel selects a normalized 50/50 dry/wet mix;
off selects 100% wet. The graph crossfades those choices over 10 ms, without
adding a second full-amplitude dry signal. Zero Compression fades the compressor
contribution to identity over 10 ms even when Attack, Sustain or Output keep the
device active.

## Validation status

Implementation and documentation were inspected without executing a build,
tests or audio measurements at the user's request. Existing tests have not been
updated for these changed contracts. Listening, GR-target convergence, makeup
recovery, timing and CPU measurements remain required before audio-quality or
performance claims.
