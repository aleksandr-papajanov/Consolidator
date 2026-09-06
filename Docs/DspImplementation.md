# DSP implementation structure

## Ownership

This document covers source organization and hot-path implementation choices.
Signal order, lifecycle and runtime policy remain specified in
[`DspRuntimeFoundation.md`](DspRuntimeFoundation.md).

Every processor has its own `.h` contract and `.cpp` implementation. Composite
processors include only their owned child processors; the runtime graph includes
its six device processors. The former all-processors header is removed, with no
compatibility umbrella. Native snapshot layout and Managed/Max bindings are unchanged.

Shared components have narrow responsibilities:

| Component | Responsibility |
| --- | --- |
| `DspParameterRamp` | 10 ms scalar interpolation; small per-sample operations are inline. |
| `DspGainRamp` | Manual gain ramp applied to selected audio channels. |
| `DspAutoGain` | Partial RMS compensation shared by Saturation and Transient Shaper. |
| `DspMath.h` | One-pole time coefficient and dB-to-amplitude conversion. |
| `DspOnePole.h` | Prewarped TPT low/high-pass sections used by Polish. |
| `DspAntiderivative.h` | First-order ADAA endpoint evaluation and midpoint limit. |
| `DspProcessingPrimitives.h` | Non-owning selected-channel view, peak/mean-square and gain application. |
| `DspParallelStage` | Device crossfade and parallel branch storage. |
| `DspBiquadCascade` | Compiled EQ coefficients, channel histories and filter crossfades. |

Channel selection counts only non-null selected channels. Detector link policies
remain algorithm-specific: compression uses mean-square, while saturation and
transient shaping use maximum magnitude. Shared helpers do not unify those
different sonic choices. The unused frequency-band runner has been removed;
frequency-plan data remains part of the existing runtime contract.

ADAA stores the previous physical sample, evaluates both endpoints using current
normalization/parameters and retains the existing near-equal midpoint threshold.
Thick and Air keep separate transfer functions. Saturation retains its specialized
rationalized antiderivative quotient rather than using the general difference
formula, which would lose its cancellation resistance.

## Reduced hot-path work

- A settled 100% wet device processes main audio directly, with a local frame so
  detector-route changes do not escape the device. Crossfade/parallel operation
  retains preallocated copies. Capacity validation and reset-after-inactivity
  semantics apply to both paths. No new allocation is introduced.
- Saturation uses one channel loop and per-channel crossover/ADAA history. Shape
  strength and normalization factor are calculated once per sample, not per band.
  At Split endpoints only the audible nonlinear path is evaluated; at zero Drive
  neither needs nonlinear evaluation. Crossovers and all previous-sample histories
  still advance, including for the inaudible path, so automation can resume it.
- Thick evaluates the polynomial directly inside `[-1,1]`, without evaluating
  the exponential overload extension at zero distance. The outside transfer and
  its antiderivative are unchanged.
- Limiter converts a settled Target to linear gain once per block. During a ramp
  it preserves per-sample ceiling calculation, including immediate downward
  target enforcement and the final hard clamp. After the ramp ends mid-block,
  remaining samples use the target ceiling, not the block-start ceiling.
- Parameter-ramp sample advancement and accessors are inline, retaining the
  existing return-current-then-advance convention and exact final-target snap.
- Settled biquad cascades run only the current path without per-sample transition
  ratio/interpolation work. Blocks containing filter transitions retain the
  existing two-path processing and state handover.

Detector buffers intentionally remain independent copies, including for neutral
EQ: downstream processors modify main audio while reading detector samples.
Stateful detectors and filter histories are not skipped merely because a knob is
zero. No approximate log/exp, reduced control rate, oversampling changes or DSP
coefficient retuning are introduced by this refactor.

## Review and validation boundary

Review covers direct consumers, per-channel state, parameter transitions,
inactivity reset, realtime ownership and project source lists. New AutoGain
translation-unit references are present in both production and standalone Native
projects. The existing Leveler test includes its dedicated processor header;
test scenarios and assertions are unchanged. Native graph identity fixtures use
the normalized neutral Width value `1`.

No build, tests, benchmark, spectral comparison or host listening was performed.
The refactor intends to preserve sound, but bit-identical output is not promised:
removing dry/wet identity arithmetic and regrouping shared calculations can change
floating-point rounding. CPU improvement is not measured. Subsequent authorized
validation should cover Split/Drive automation, wet/bypass transitions, mono and
stereo, limiter Target ramps ending inside a block, and Thick attacks.
