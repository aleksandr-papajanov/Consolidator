# Polish harmonic enhancement

## Topology

Polish processes Thick followed by Air. Each processor retains its input and
adds only its filtered enhancement. There is no 50/50 average of two complete
dry-plus-effect branches, so neither knob is halved by the other branch.
The graph's separate 10 ms activity/bypass crossfade surrounds the whole device.
Parameters remain the existing `polish.thick` and `polish.air` in `0..1`.

These are original DSP designs. The bass-harmonic goal is inspired by the
[Renaissance Bass manual](https://assets.wavescdn.com/pdf/plugins/renaissance-bass.pdf);
the separate warmth and shimmer goals are described in the
[Revival documentation](https://docs.slatedigital.com/VMR/Revival.html).
The transfer functions below are our own; this implementation does not claim to
clone either product.

## Shared primitives

Frequency isolation uses cascades of two prewarped TPT one-pole sections.
The direct Air shelf uses one high-pass section summed with dry, producing a
monotonic shelf rather than a phase-rotated two-section high-pass addition.
Their nominal section frequencies are limited to 40% of sample rate.
Each channel has independent filter and nonlinear history; only control envelopes
are linked. All state is scalar or fixed-size arrays, prepared before processing.

The harmonic generator is an asymmetric rational residual:

```text
H(x) = (0.5*x*x + 0.12*x*x*x) / (1 + x*x)
```

The even term supplies predominantly second and smaller higher even harmonics;
the odd term supplies odd harmonics as well as some fundamental contribution.
Post-filtering attenuates the fundamental and removes the even term's DC.
The numerator is not an uncontrolled polynomial at high input amplitudes.

First-order ADAA evaluates the difference of its antiderivative
`0.5*(x-atan(x)) + 0.06*(x*x-log(1+x*x))` between adjacent samples.
Near equal endpoints use the continuous midpoint limit to avoid cancellation.
Both endpoints use the current normalization scale. ADAA reduces aliasing but
does not make the exciter alias-free; there is no oversampling or true-peak claim.
Only the generated branch has this averaging response; dry audio is undelayed.

## Thick

```text
input -> LP 120 Hz -> linked 200 ms energy normalization
      -> asymmetric harmonic generator -> HP 100 Hz -> LP 700 Hz
      -> intensity -> add to original input
```

The normalization scale is sqrt(2 * linked energy), floored at 1e-5 amplitude.
The startup estimate is divided by accumulated exponential weight, avoiding
an artificially small initial normalization scale.
Intensity increases the wet multiplier from zero to six. There is no direct
low shelf or multiplication of the original sub-bass. A 55 Hz source produces
new content around 110, 165, 220 Hz and beyond, subject to the filters and
waveshaper; the algorithm does not track pitch or guarantee those harmonic levels.
Some fundamental leakage remains because the filters have finite slopes.
Additional harmonics can still increase peaks and consume headroom.

## Air

```text
input -> HP 3 kHz -> LP 8 kHz -> linked 200 ms normalization
      -> asymmetric harmonic generator -> HP 7 kHz
      -> dynamic attenuation -> subtle side enhancement -> add to dry
```

A first-order shelf above 9 kHz complements the generated harmonics. At maximum
Air and without dynamic attenuation, its asymptotic gain is 1.6 (about +4.1 dB).
The harmonic branch multiplier reaches four at maximum Air.
Both additions pass through the same attenuation, including their side component.

Harshness combines the ratio of source-band energy to full-band energy with a
2 ms source-band burst detector relative to the 200 ms reference. Both averages
are normalized by their accumulated weights during startup, preventing false
bursts caused solely by their different time constants. Attenuation starts when
source-band energy exceeds 15% of full-band energy or the fast/slow band-energy
ratio exceeds three; ordinary levels below those thresholds receive no ducking.
This makes
attenuation depend on spectral concentration and sudden upper-mid peaks rather
than only on absolute dBFS. Attenuation responds in 1 ms and recovers in 80 ms.
It controls only the added signal: it never de-esses or ducks the original dry
audio. The 1e-5 normalization floor limits excitation of near-silence.

The added stereo side is 8% of the wet-channel difference, applied with opposite
signs. It cancels in a mono sum and is disabled for single-channel processing.
Identical left/right inputs therefore do not acquire synthetic stereo width.

## Validation status

Only source inspection was performed. No build, tests, spectral analysis or
listening was run, and existing tests were left unchanged at the user's request.
Harmonic balance, bass fundamental leakage, sibilance behaviour, sample-rate
response, alias rejection and CPU cost require subsequent audio validation.
