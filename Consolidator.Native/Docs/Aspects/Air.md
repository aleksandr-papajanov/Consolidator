# PolishAirProcessor

[Native algorithm](../../External/Dsp/Processors/Air/PolishAirProcessor.cpp),
[constants](../../External/Dsp/Processors/Air/PolishAirProcessor.h).
The VST3 compiles the same Native model. Air is derived from Revival Shimmer;
exact reference equivalence is not a goal. Amount is 0..1, default zero, with a
10 ms ramp. Native configuration is immutable; VST3 exposes only Air and Bypass.

## Linear contour

The additive contour contains direct gain -0.0684, single-pole HP additions at
6370 Hz with gain -1.48 and 10860 Hz with gain 4.325, plus serial bells at
4500 Hz/+1.5 dB/Q 1.4 and 6400 Hz/-0.5 dB/Q 2.2, subtracting their dry input.
The former 10848/10877 Hz HP additions were merged into one weighted section.
The 5 Hz baseline HP blends in with min(4*amount,1). Neutral Amount is exact identity.

The linked detector takes the larger channel peak immediately, refreshes a one-second
hold on recurring peaks and releases over two seconds. It controls only the
level contour. Hold avoids stationary-tone modulation and its harmonic comb.

The five-point level table is replaced by a bounded quadratic:

~~~text
u = clamp((peakDb + 30)/30, 0, 1)
linearGain = min(1, 1 + u*(0.0086 - 0.3792*u))
levelLoss = 1 - linearGain
levelAddition = -levelLoss * (3*HP10000^3(input) + input - bell10500(input))
~~~

The 10500 Hz bell has -12 dB gain and Q 1.3. The unrounded quadratic fit had
0.00586 RMS error in coefficient across the old -30..0 dB curve. Peak dB and
linearGain update only when the held peak changes. Zero/lower-level input retains
unity coefficient; the high endpoint is 0.6294. No runtime search or fitting.

## Harmonic path and Amount

The surviving odd branch is HP1000 -> cube -> HP13120 twice -> gain -0.038.
This cubic still supplies H3 and fundamental/intermodulation terms. TPT frequencies
cap at 40% of sample rate. Weak fifth/seventh corrections and the small four-section
square/H2 branch were removed after manual comparison; no added saturation stage.

Amount is now a direct wet multiplier. The former exponent 1.01615 was rounded to
one, eliminating knob power calls. Output is dry + baseline addition + amount times
linear contour, level addition and odd branch. Filter and detector histories remain
warm at zero Amount; bypass/activity for the combined Polish device belongs to its
runtime stage. The isolated VST3 bypass ramps its amount toward zero.

## Lifecycle and runtime

Prepare compiles fixed filters and timing/hold coefficients. Reset clears all
channel histories, peak/hold and the cached unity level coefficient, preserving
the amount target. Process is one sample loop with no allocation, lock, logging,
I/O or fitting. Channel mode selects only non-null requested buffers.
The VST3 no longer compiles the unused DspBiquad.cpp.

Manual comparison and limitations: [findings](AirFindings.md).
[DSP rules](../Rules.md) prohibit automated audio tests.

## Code map

- PrepareFilters compiles fixed HP and bell coefficients.
- UpdateInputLevel owns the linked peak hold, release and cached contour coefficient.
- ProcessLinearAddition owns the fixed and level-dependent contours.
- ProcessOddHarmonics owns the cubic branch with independent per-channel filter history.
- ProcessChannelSample mixes baseline, linear and harmonic additions with the Amount ramp.

The model is complete. This organization refactor changes source layout and method boundaries;
control ranges, constants, state update order and signal routing are retained.
