# Filter definitions

`StateValueDefinitions` owns the filter definitions used to construct state.
Its `EqualizerDefinitions` and `DetectorDefinitions` are the single source
for available parameters, ranges and initial values. Wire names are owned by
`Protocol/Encoding/FilterProtocolNames`. VST3 presentation mirrors only the
presentation capabilities needed to bind controls.

The standard Equalizer layout is:

```text
Gain, Tilt, LowShelf, HighShelf, Bell, Bell, Bell
```

The detector layout for Input Leveler, Saturator and Compressor is:

```text
LowShelf, Bell
```

All frequency-capable filters use `20..20000 Hz` and `-24..24 dB` gain.
Editable Q is available only on Bell filters and uses `0.1..10`, default
`0.707`. Shelf and Tilt filters use fixed Q `0.707`; it is not an editable
state parameter. Their default frequencies are `1000, 100, 10000, 1000, 2000,
4000 Hz` for the frequency-capable EQ slots and `100, 1000 Hz` for detectors.
Gain filters expose only gain. VST3 uses these local definitions together
with the parameter bindings; no filter catalog or analyzer state frame is
transported.

## Biquad contract

VST3 curve presentation and Native audio processing use the same
normalized formulas. Frequency is limited to `0.499 * sampleRate` before
coefficient calculation. For gain `G`, `A = 10^(G / 40)`; Bell, LowShelf and
HighShelf use the same `omega`, `alpha = sin(omega) / (2Q)` and normalized
RBJ-style coefficients. Gain uses the equivalent linear multiplier
`10^(G / 20)`. Tilt is a two-stage cascade of LowShelf at `-G` followed by
HighShelf at `+G`.

Native processes every stage as transposed direct form II with independent
left and right state. Parameter, filter-activity, bank-activity and Equalizer
bypass changes compile a new stable cascade and crossfade the old and new
outputs over 10 ms; raw coefficients are never interpolated.

## Native filter primitives

Reusable filter designs and per-channel processing live in
`Audio/Primitives/Filters`. `FirstOrderHighPass` owns one channel's prepared
first-order state; `BiquadFilter` combines shared coefficients with independent
left/right histories. `DesignHighPass`, `DesignBell` and `DesignShelf` provide
coefficient designs. `BiquadCascade` composes these biquad filters for each
parameterized filter path; algorithms may also compose the primitives directly.
