# PolishThickProcessor

[Native algorithm](../../Audio/Algorithms/Thick/PolishThickProcessor.cpp),
[constants](../../Audio/Algorithms/Thick/PolishThickProcessor.h).
The main and research VST3 adapters compile this same Native source. Thick is a bass harmonic enhancer,
not an exact R-Bass copy. Amount is 0..1, default zero, with a 10 ms amplitude ramp.
Native constants are immutable; the VST3 exposes only Thick and Bypass.

## Body and level function

Two prepared bells form the ordinary body: 81 Hz/+14.68 dB/Q 0.725 and
34.15 Hz/-9.43 dB/Q 1.67. The positive-level endpoint uses 19 Hz/+9.57 dB/Q 1.4
and 52 Hz/-16.67 dB/Q 1.15. These are parallel filtered copies of the input.
The endpoint mix is clamp(inputRmsDb/10, 0, 1), smoothed over 20 ms.
Its logarithm is evaluated only when RMS amplitude exceeds one.

The former seven-point body-mix table is replaced by one rational function:

~~~text
x = sqrt(min(linked RMS amplitude, 1))
bodyMixTarget = clamp(0.0684 + (1-x)*(1.599 - 2.071*x + 2.404*x*x)/(1+0.87*x),
                      0.0684, 1)
bodyWet = amount * smooth20ms(bodyMixTarget) * (levelFilteredBody - dry)
~~~

The function was fitted offline across the old -30..0 dB curve. Unrounded fit RMS
error was 0.00289 in mix coefficient. Coefficients describe a function, not anchors.
The denominator is positive on the full input domain. Quiet upper mix is one;
large signals retain the 0.0684 body floor. No runtime fitting or table search.

## Source and harmonic generation

Source is HP20 followed by two LP60 sections. A linked 200 ms mean-square reference
uses startup weight correction. Scale is the larger of RMS, a 1/120 ms source peak
follower and 1e-5. Full-band level and bass/full-band energy ratio prevent tiny HF
leakage from being normalized into strong distortion. These controls are retained.

Normalized source x is clamped to [-1,1]. Two bounded, opposite-polarity one-sample
feedback loops use g=0.56+0.22*amount; their odd difference removes the fundamental,
while a 0.03 even sum remains. The dense addition retains every term H2..H159:

~~~text
D(x) = 0.12 * sum(r^(m-1) * (T(2m+1,x) + 0.03*T(2m,x)), m=1..79), r=0.97
~~~

A finite geometric-series formula replaces 79 harmonic-pair iterations.
Chebyshev fast doubling computes T79/T80 in seven steps, then T158..T161.
The common denominator is 1-2*r*T2+r*r, bounded below by (1-r)^2.
r^79 prepares once. The finite end correction is retained, so high orders and
intentional Nyquist folding remain; this is not an infinite-series approximation.

A separate half-scale H2 branch supplies low-level colour. LP120 sub source adds
H2 + 0.56*H3 + 0.065*H4 + 0.12*H5. Each branch has its own HP60/LP18k states.
TPT filter frequencies cap at 40% of sample rate. Rich drive is 0.9816*level^24;
low-level drive is 0.0184*level. Intensity is amount^2*(1+25*amount)*bassPresence.
Normalization, inverse scale and loop gain are shared within each sample.

## Wet dynamics and ceiling

Filtered rich wet uses 200/20 ms energy references. Calibration is bounded to
0.5..3, upward gain to 1..1.5, burst gain to at most one. The total target divides
by max(intensity,1), clamps to 0.01..4 and smooths with 1/80 ms times.
The sub contribution retains its independent 0.1 drive.

A 30 ms bass peak/RMS crest controller starts above crest 1.25. Its target is
max(0.35,(1.25/crest)^0.75), smoothed over 0.5/80 ms. Body receives 35% of its
amount-weighted reduction; rich wet receives the full amount-weighted reduction.
Dry and explicit sub content are retained. Output morphs by Amount toward a
hard +/-1 ceiling. Amount zero remains exact dry identity.

## Lifecycle and final simplification

Prepare allocates four body buffers, compiles filters and time coefficients.
Fixed biquads are reinitialized before compilation so repeated Prepare at a new
rate cannot reuse stale coefficients. Reset clears filter/detector histories and
snaps control targets. Process has no allocation, locking, I/O or fitting.

The four body filters remain: their gains are substantial. Sparse low-level/sub
harmonics, crest control and ceiling also remain. Tiny numerical detail was rounded;
no oversampling, changed alias suppression or extra signal path was introduced.
Manual comparison details and remaining limitations: [findings](ThickFindings.md).
DSP automated tests remain prohibited by [rules](../Rules.md).

## Code map

- PrepareBodyFilters and PrepareSourceAndDynamics prepare all storage and coefficients.
- ProcessBodyPaths renders the two fixed body copies before dry input is changed.
- AnalyseSource returns linked SourceAnalysis; UpdateSourceControls advances normalization and drive.
- UpdateCrestGain and UpdateBodyLevelMix own the two level-control histories.
- GenerateHarmonicBands renders rich, low-level and sub additions into fixed-size local records.
- UpdateWetGain calibrates the rich branch; MixOutputSample combines body/bands and applies the ceiling.

The model is complete. This organization refactor changes source layout and method boundaries;
control ranges, constants, state update order and signal routing are retained.
