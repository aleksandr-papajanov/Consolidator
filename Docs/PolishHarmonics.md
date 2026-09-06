# Polish harmonic enhancement

## Scope and references

Polish processes Thick followed by Air. Each adds its enhancement to the incoming
audio; there is no averaging of two dry-plus-wet branches. Existing parameters
`polish.thick` and `polish.air` remain `0..1`, with 10 ms ramps. The graph's
separate activity/bypass crossfade surrounds the device. No ABI or UI change.

These are original designs, not emulations of proprietary algorithms:

- [R-Bass](https://assets.wavescdn.com/pdf/plugins/renaissance-bass.pdf) documents
  bass-derived harmonics and independent original-bass/intensity controls.
- [MaxxBass](https://assets.wavescdn.com/pdf/plugins/maxxbass-v16-update.pdf)
  documents harmonic spectral decay and upward compression of generated content.
- [Waves patent US5930373A](https://patents.google.com/patent/US5930373A/en)
  describes a recursive multiplicative generator with dynamic feedback/output
  control. Our feedforward polynomial generator does not implement that circuit.
- [Revival](https://support.slatedigital.com/hc/en-us/articles/211730817-Revival)
  describes analog-inspired tone and shimmer, but does not specify transfer
  functions, filter frequencies or a de-esser. The Air topology below is our choice.

## Shared runtime properties

Isolation filters use two cascaded prewarped TPT one-pole sections, except the
single-section Thick rumble filter and Air shelf branches. Listed frequencies
are section frequencies, not the -3 dB points of the complete cascades, and are
limited to 40% of sample rate. Channels own independent filters and nonlinear
history; selected channels share maximum-channel energy detectors and gains.
Unselected channels neither contribute to detectors nor receive processing.

The RMS normalization reference is `sqrt(max(2 * meanEnergy, 1e-10))`. Thick
also uses a fast source-peak follower as described below. Exponential energy estimates
are divided by accumulated weights during startup. Both nonlinear generators
use first-order ADAA, with both endpoints evaluated at the current scale and
current shape settings. Nearby endpoints use the continuous midpoint limit.
Dry remains undelayed; only the harmonic contribution has ADAA averaging.
This reduces aliasing, but is not oversampling or an alias-free guarantee.
The endpoint evaluation is shared in `DspAntiderivative.h`; transfer functions
remain local to their processors. TPT sections live in `DspOnePole.h`, while
time coefficients are shared through `DspMath.h`. See
[`DspImplementation.md`](DspImplementation.md) for refactoring and CPU-work scope.

All history is scalar or fixed-size arrays. No allocation, locks, callbacks or
Max/Managed operations occur during processing. Reset clears all filter and
detector history and restores controller gains. Zero amount adds exactly zero.

## Thick

```text
input -> HP 20 Hz -> LP 50 Hz -> linked RMS/peak normalization
      -> H2/H3/H4 generator -> HP 50 Hz -> LP 250 Hz
      -> level calibration + bounded upward compression -> intensity -> add to dry
```

Within normalized `-1..1`, the harmonic function is:

```text
H(x) = 0.9*x² + (0.35 + 0.35*amount)*(x³ - 0.75*x)
              + (0.1 + 0.15*amount)*(x⁴ - x²)
```

Second-harmonic weight remains 0.9; reducing the third/fourth weights makes its
relative contribution dominant. The 250 Hz output low-pass further reduces upper
bass coloration. Post-filter calibration and attack protection still operate
within their existing gain limits.

For a unit-amplitude sinusoid the bases supply second, third and fourth harmonics
(and DC in the even terms), without a fundamental term. This is not a pitch
tracker: other amplitudes, transients and polyphonic signals also produce
fundamental leakage, higher harmonics and intermodulation. Outside `-1..1`, each
end approaches a finite plateau with an exponentially decreasing slope, retaining
continuous value and first derivative at the join. With edge `e = sign(x)`,
distance `d = abs(x)-1` and width `w = 0.2`, the extension is
`H(e) + e*H'(e)*w*(1-exp(-d/w))`. The ADAA integral includes this extension;
the previous unbounded tangent continuation is removed. Output high-pass filtering
removes DC; its lower cutoff retains more of low notes' second harmonics.

The post-filter wet detector has 200 ms and 20 ms windows, measured before gain:

```text
calibration = clamp(sqrt(sourceMean / max(wetMean, 1e-10)), 0.5, 3)
upward      = clamp((wetMean / wetFast)^0.25, 1, 1.5)
burst       = sqrt(min(1, wetMean / wetFast))
gainTarget  = clamp(calibration * upward * burst, 0.25, 4)
intensity   = amount * (0.46875 + 1.03125*amount)
```

Calibration compensates the actual filtered harmonic level, bounded to avoid
chasing missing content. Upward compression raises portions below the moving
wet reference, approximately 2:1 relative to a fixed reference and capped at
+3.5 dB. Above the reference, burst reduction reduces the target rather than
carrying boosted tail gain into a new attack. Targets return to unity when source
mean or fast wet energy is below `1e-10`; there is no hard audio gate. Gain
decreases in 1 ms and increases in 80 ms, with a maximum target of four.
At maximum amount the steady wet RMS aims at 1.5 times source-band RMS, subject
to calibration limits and smoothing; this is not a guaranteed output loudness.
The intensity is 1.25 times the previous mapping (about +1.9 dB in the added
branch); detector gains and attack protection are unchanged. The stronger
addition can nevertheless drive the downstream limiter harder.

The normalization scale is the larger of the 200 ms RMS reference and a linked
absolute-source follower with 1 ms attack and 120 ms release. This catches new
attacks after quiet passages before the slow reference has caught up. Reset
clears this follower too. The 50 Hz source setting targets deeper bass material;
it is not a calibrated equivalence to R-Bass's Freq control. These changes reduce
attack-overload risk but do not establish that the reported click is eliminated.

Amount therefore changes harmonic balance and intensity, not just a fixed wet
multiplier. Original bass is not boosted by a shelf or passed through this
compressor. Added harmonics can still increase peaks and consume headroom.

## Air

Air separates audible tonal lift from dynamic nonlinear coloration:

```text
input -> single HP 9 kHz   -> 1.2*amount ---------------------+
      -> single HP 15 kHz  -> 0.85*amount² -------------------+-> add to dry
      -> HP 3 kHz -> LP 6.5 kHz -> normalized harmonics       |
         -> HP 9 kHz -> harshness attenuation -> 2.6*amount² +
```

The two single-pole high-pass additions form a broad high shelf with extra top
lift. At maximum amount their combined Nyquist gain, including dry, is 3.05
(about +9.7 dB); at halfway it is 1.8125 (about +5.2 dB). These are shelf-only
endpoint gains, not gains throughout the audible band. The shelf never goes
through the harshness controller, so bright material does not turn off the
entire Air effect. It can still emphasize existing sibilance at high settings.

The separate smooth harmonic residual is
`H(x) = x²*(0.5 + 0.12*x)/(1 + x²)`, with integral
`0.5*(x-atan(x)) + 0.06*(x²-log(1+x²))`. It retains some fundamental
contribution; output filtering is not perfect harmonic extraction.

Harshness combines 200 ms source/full-band energy density and a 2 ms source-band
burst estimate. Ducking starts above density 0.15 or fast/slow energy ratio three:
`attenuationTarget = 1/(1 + 8*max(density-0.15,0) + 0.75*max(burstRatio-3,0))`.
Attack is 1 ms, recovery 80 ms. Only generated harmonics are attenuated.
The harmonic contribution grows quadratically with Amount so lower settings
emphasize tonal lift before stronger excitation.

Stereo adds 8% of the harmonic wet-channel difference with opposite signs.
This addition cancels in mono and is disabled for single-channel processing;
neither the dry audio nor the shelves are widened.

## Validation status

Only source review was performed. No build, tests, spectral analysis or listening
was run; existing tests remain unchanged at the user's request. Harmonic balance,
fundamental leakage, automation, transients, silence, sibilance, sample-rate
response, alias rejection and CPU cost still need audio/host validation.
The new coefficients are design choices, not measurements of R-Bass or Revival.
