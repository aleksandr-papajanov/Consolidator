# Polish harmonic enhancement

## Scope and references

Polish processes Thick followed by Air. Each adds its enhancement to the incoming
audio; there is no averaging of two dry-plus-wet branches. Existing parameters
`polish.thick` and `polish.air` remain `0..1`, with 10 ms ramps. The graph's
separate activity/bypass crossfade surrounds the device. No ABI or UI change.

Thick remains an original design. Air is a black-box behavioral model fitted
from user-authorized measurements of Revival Shimmer:

- [R-Bass](https://assets.wavescdn.com/pdf/plugins/renaissance-bass.pdf) documents
  bass-derived harmonics and independent original-bass/intensity controls.
- [MaxxBass](https://assets.wavescdn.com/pdf/plugins/maxxbass-v16-update.pdf)
  documents harmonic spectral decay and upward compression of generated content.
- [Waves patent US5930373A](https://patents.google.com/patent/US5930373A/en)
  describes a recursive multiplicative generator with dynamic feedback/output
  control. Thick uses an original bounded dual-loop variation of that general
  topology, not the patented circuit or its loudness-control equations.
- [Revival](https://support.slatedigital.com/hc/en-us/articles/211730817-Revival)
  describes analog-inspired tone and shimmer but does not specify transfer
  functions or filter frequencies; the Air model parameters are inferred from
  measured input/output behavior rather than that description.

## Shared runtime properties

Isolation filters use two cascaded prewarped TPT one-pole sections, except the
single-section Thick rumble filter and Air shelf branches. Listed frequencies
are section frequencies, not the -3 dB points of the complete cascades, and are
limited to 40% of sample rate. Channels own independent filters and nonlinear
history; selected channels share maximum-channel energy detectors and gains.
Unselected channels neither contribute to detectors nor receive processing.

The RMS normalization reference is `sqrt(max(2 * meanEnergy, 1e-10))`. Thick
also uses a fast source-peak follower as described below. Exponential energy estimates
are divided by accumulated weights during startup. The Air nonlinear generator
uses first-order ADAA, with both endpoints evaluated at the current scale and
current shape settings. Nearby endpoints use the continuous midpoint limit.
Thick intentionally uses a one-sample recursive generator without ADAA because
its folded high orders are part of the selected texture. The shared endpoint
evaluation lives in `DspAntiderivative.h`; transfer functions remain local to
their processors. TPT sections live in `DspOnePole.h`, while
time coefficients are shared through `DspMath.h`. See
[`DspImplementation.md`](DspImplementation.md) for refactoring and CPU-work scope.

All history is scalar or fixed-size arrays. No allocation, locks, callbacks or
Max/Managed operations occur during processing. Reset clears all filter and
detector history and restores controller gains. Zero amount adds exactly zero.

## Thick

```text
input -> fixed bell 81.038 Hz/+14.6765 dB/Q 0.724975
      -> bell 34.1538 Hz/-9.42724 dB/Q 1.66623 ------------------------+
      -> above-0-dB endpoint: bell 19 Hz/+1.272 dB/Q 1.4,
         bell 52 Hz/-3.188 dB/Q 1.15 ----------------------------------+
      -> HP 20 Hz -> LP 60 Hz -> linked RMS/peak normalization             |
      -> full-band/bass-band presence control                             |
      -> harmonic generator -> HP 60 Hz -> LP 18 kHz                      |
      -> level calibration + bounded upward compression -> intensity -----+-> add to dry
```

For normalized input `x`, the generator runs two bounded one-sample feedback
loops at opposite input polarities:

```text
p[n] = clamp( x[n] + g*x[n]*p[n-1], -4, 4)
n[n] = clamp(-x[n] - g*x[n]*n[n-1], -4, 4)
g    = 0.56 + 0.22*amount
H[n] = 0.5*(p[n]-n[n])-x[n] + 0.03*0.5*(p[n]+n[n])
D(x) = 0.12*sum(w[k]*Tk(x), k=2..159)
w[2m+1] = 0.97^(m-1)
w[2m]   = 0.03*0.97^(m-1)
```

The difference of the loops removes their shared normalized fundamental and
makes odd orders dominant. The quieter sum retains even orders. A parallel
Chebyshev bank extends through H159, with even orders about 30.5 dB below
adjacent odd orders. Each odd/even pair decays by 3%, reaching approximately
-20.7 dB at H159. The output filter adds only the final high-frequency rolloff.
Recursive and explicit orders
above Nyquist fold back into the measured band and form side products around
the direct harmonics. This aliasing is intentional in Thick and is the source
of its dense saturation texture. The feedback clamp is a finite-safety bound,
not a separate fallback signal path. This is not a pitch tracker: transients and
polyphonic signals also produce fundamental leakage and intermodulation.
Output high-pass filtering removes DC and bass below the reference frequency;
the 18 kHz low-pass retains the dense high-order shelf while suppressing the
Nyquist edge. As with the other TPT filters, its configured frequency is capped
at 40% of sample rate, or 17.64 kHz at 44.1 kHz.

The post-filter wet detector has 200 ms and 20 ms windows, measured before gain:

```text
calibration = clamp(sqrt(sourceMean / max(wetMean, 1e-10)), 0.5, 3)
upward      = clamp((wetMean / wetFast)^0.25, 1, 1.5)
burst       = sqrt(min(1, wetMean / wetFast))
bassPresence     = clamp(sqrt(sourceMean / max(fullBandMean, 1e-10)), 0, 1)
fullBandPeak     = max(abs(input), release120ms(fullBandPeak))
fullBandLevelRaw = max(sqrt(2*fullBandMean), fullBandPeak)
fullBandLevel    = clamp(fullBandLevelRaw, 0, 1)
lowLevelDrive    = 0.0184*fullBandLevel
richLevelDrive   = 0.9816*fullBandLevel^24
intensity        = amount² * (1 + 25*amount) * bassPresence
subSource        = lowpass(bass, 120 Hz)
subHarmonics     = H2 + 0.56*H3 + 0.065*H4 + 0.12*H5
gainTarget       = clamp(
    calibration * upward * burst / max(intensity, 1), 0.01, 4)
harmonics        = intensity * (
    gain*(richLevelDrive*rich + lowLevelDrive*low) + 0.1*subHarmonics)
enhanced         = input + bodyWet + harmonics
crest            = peakRelease30ms(abs(bass)) / max(sqrt(2*bassMean), eps)
crestGain        = clamp((1.25/crest)^0.75, 0.35, 1)
controlledBody   = bodyWet * lerp(1, crestGain, 0.35)
controlledRich   = richHarmonics *
    smooth(crestGain, attack=0.5ms, release=80ms)
controlledWet    = controlledBody + controlledRich + subHarmonics
controlled       = input + controlledWet
output           = controlled + amount*(clamp(controlled, -1, 1) - controlled)
```

The analyzer's nominal 0 dB stimulus is calibrated as a sine with sample peak
`1.0`. The final stage is the measured zero-dBFS ceiling rather than a fitted
fundamental-gain trim. At full amount it clips the enhanced signal at `+/-1`;
the resulting waveform itself produces the strong level-dependent odd series
seen above 0 dBFS and limits the first attack without waiting for an RMS
detector. The amount control morphs continuously between the enhanced signal
and this limited result.

Before the final ceiling, a linked crest controller compares a 30 ms bass-band
peak follower with the 200 ms bass-band reference. Crest below `1.25` leaves
the signal unchanged. Higher short bass attacks receive 4:1 compression down
to a minimum gain of `0.35`, with 0.5 ms attack and 80 ms release. Full gain
reduction applies only to the recursive and low-level rich harmonics. The body
fundamental follows 35% of that reduction, while the explicit sub H2-H5 path
remains stable. The dry input is never reduced. An isolated kick therefore
receives less distorted Thick enhancement without broadband pumping, while
the sustained low-frequency weight remains present. A stationary sine settles
to unit crest gain and retains the measured harmonic response.

Above 0 dBFS the body response morphs from the base contour toward a mild
corrective endpoint. Offline fitting reduced the former +20 dB peak and -22 dB
notch to a +1.272 dB shoulder at 19 Hz and a -3.188 dB dip at 52 Hz; the fitted
27 Hz notch reached effectively 0 dB and was removed. The endpoint compensates
fundamental leakage from the harmonic generator rather than creating a new
resonant body. The morph is zero at and below 0 dBFS and reaches its endpoint
at +10 dBFS; no filter coefficients are compiled on the audio thread.

Calibration compensates the actual filtered harmonic level, bounded to avoid
chasing missing content. Upward compression raises portions below the moving
wet reference, approximately 2:1 relative to a fixed reference and capped at
+3.5 dB. Above the reference, burst reduction reduces the target rather than
carrying boosted tail gain into a new attack. Targets return to unity when source
mean or fast wet energy is below `1e-10`; there is no hard audio gate. Gain
decreases in 1 ms and increases in 80 ms, with a maximum target of four.
When Intensity exceeds unity, gain calibration compensates the recursive and
low-level paths before they are added. Their steady generated RMS therefore
remains near source-band RMS, subject to upward compression, calibration limits
and smoothing; this is not a guaranteed output loudness. The explicit sub path
uses its own fixed drive so its level remains stable on sustained bass.
Intensity below unity is not compensated, so the control still fades every
harmonic contribution toward zero.

Level control changes spectral shape as well as total THD. `richLevelDrive`
controls the recursive H159 odd-dominant generator and is effectively absent
below -3 dBFS. `lowLevelDrive` controls a separate half-scale Chebyshev H2
generator. A separately filtered sub-band source adds explicit H2 through H5
below the rich-mode transition; keeping this branch independent preserves the
sparse 1 kHz response while reproducing the denser compressed low-frequency
body. Its drive does not use the adaptive gain calibrated from the recursive
rich branch, so sustained bass does not continue rising after the measurement
window. Together these paths leave a sparse series at reduced input levels and give
approximately one decibel of THD reduction per decibel of input reduction.
Both branches have independent output-filter state. Their coefficients fit the
measured reference ratios at 0, -3, -6 and -9 dBFS. The full-band detector is
used so source-filter leakage cannot be normalized into level-independent
distortion. The generated band remains centered on the 60 Hz reference setting.

Above 0 dBFS the final zero-dBFS ceiling produces measured fundamental gains of
-1.733, -4.289 and -8.050 dB for +3, +6 and +10 dBFS input. Amount interpolates
between the pre-ceiling and limited signals and is exactly neutral at zero.

The bass-presence ratio prevents source-filter leakage from being normalized
back to full level. A high-frequency tone without meaningful energy below the
60 Hz source cutoff therefore produces only the attenuated residual, while a
real low-frequency source receives the stronger maximum intensity.

The body branch supplies the strong low-frequency transfer visible at the
maximum reference setting independently of the generated harmonics. Its two
bell sections were fitted offline against the -30 dBFS reference sweep through
500 Hz. Their interaction places the sampled composite maximum near 80 Hz and
minimum near 30 Hz. The fit has 0.371 dB RMS error across the training points.
The complex response is mixed as
`dry + amount*levelMix*(filtered-dry)`. Offline fitting supplies levelMix anchors
of 1.001 at -30, 0.646 at -18, 0.469 at -12, 0.390 at -9, 0.319 at -6,
0.212 at -3 and 0.068 at 0 dBFS. Realtime processing linearly interpolates
between them in decibels and clamps outside the measured range. Zero amount
remains exactly neutral and coefficient compilation stays outside the audio
callback.

The normalization scale is the larger of the 200 ms RMS reference and a linked
absolute-source follower with 1 ms attack and 120 ms release. This catches new
attacks after quiet passages before the slow reference has caught up. Reset
clears this follower and the crest controller. The 60 Hz source setting targets
the reference bass material; it is not a calibrated equivalence to R-Bass's
Freq control.

Amount therefore changes harmonic balance and intensity, not just a fixed wet
multiplier. Original bass is not boosted by a shelf or passed through this
compressor. Added harmonics can still increase peaks and consume headroom.

## Air

Air is the native model fitted to the measured Revival Shimmer control:

```text
input -> direct gain + three HP additions + bell contour --+-> amount^exponent -> add to dry
      -> level HP³ + level bell ----------------------------+
      -> HP source -> odd polynomial -> HP² output ---------+
      -> square -> HP⁴ top-octave output -> even gain ------+
      -> 5 Hz baseline HP blended on module activation -----+
```

The three single-pole high-pass additions and local bells reproduce the measured
complex linear response more directly than a conventional shelf. A separate
three-pole high shelf and broad bell apply the linked input-level curve only to
the upper contour: the reference's top lift contracts as a sine approaches
0 dBFS while 4–6 kHz remains substantially stable. The odd branch includes
cubic, fifth-order and seventh-order terms and stays outside that attenuation.
A much quieter square branch models the top-octave H2 residue. Amount uses a
fitted exponent so intermediate knob positions are not assumed linear in wet
amplitude.

The linked detector attacks immediately, holds for one second while recurring
peaks refresh it, and then releases over 2 seconds. The hold prevents its gain
from creating a false harmonic comb on stationary tones. It controls only the
linear additions. Filter and detector histories are
fixed per-channel state, and zero Amount is an exact identity. Detailed
measurement assumptions and the fitting command are in
[`RevivalShimmerEmulation.md`](RevivalShimmerEmulation.md).

## Validation status

The body coefficients are generated by the offline measurement optimizer and
were validated with a fresh candidate sweep. Across the -30 through 0 dBFS
levels and frequencies through 500 Hz, level-dependent mixing reduced gain
RMSE from 4.01 dB to 0.71 dB and maximum absolute error from 12.49 dB to
1.70 dB. The final ceiling matches reference points at 1 kHz and above with
0.02 dB RMS and 0.03 dB maximum error. Across all positive frequencies,
ceiling modeling and residual-aware positive-body fitting reduced gain
RMSE from 1.61 dB to 0.68 dB and maximum error from 3.87 dB to 2.04 dB.
Full-matrix gain RMSE is 0.60 dB. The largest remaining positive-level error is around 80 Hz at
+3 and +6 dBFS, where level-dependent harmonic fundamental leakage is not part
of the filter-only optimizer model. Harmonic
balance, fundamental leakage, automation, transients, silence, sibilance,
sample-rate response, alias rejection and CPU cost still need audio/host
validation in Plugin Doctor.

The Air defaults are fitted to the settled 44.1 kHz solo Revival Shimmer
measurements and validated with a fresh VST3 candidate sweep. Remaining
validation boundaries and fit metrics are documented in
[`RevivalShimmerEmulation.md`](RevivalShimmerEmulation.md).

The coefficients used by Thick are exposed as a pre-`Prepare()`
`PolishThickTuning` value through `SetTuningBeforePrepare()`. Production uses
its documented defaults. The offline
measurement executable can replace that value before allocation and processing
so its full optimizer evaluates the actual DSP implementation; tuning is never
changed, allocated or optimized from the realtime audio path.
Air exposes the equivalent pre-`Prepare()` `PolishAirTuning` contract and uses
the independent `air` optimization stage.
