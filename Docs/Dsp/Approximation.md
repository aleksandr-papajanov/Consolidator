# Equalizer bank approximation

`Commit` folds one non-empty equalizer bank into bank `0`. The state tree is the
only input: live audio capture and streamed FFT presentation frames do not
participate.

For every target instance, Managed snapshots the active filters of the source
bank and asks `FftAnalyzer` once to calculate their 512-point logarithmic
frequency response over `20..20000 Hz`. Approximation uses the same direct
biquad frequency-response calculation for both the target and every NLopt
candidate, so the optimizer does not compare an FFT-truncated impulse response
with an analytical response. COBYLA has a budget of `10000` objective
evaluations and optimizes frequency parameters on a logarithmic scale. The
result is accepted when NLopt completes successfully and produces a finite
objective value; no dB accuracy gate rejects a finite approximation. The
biquad contract matches the Max analyzer and Native DSP.
The instance's prepared sample rate is used when available; otherwise it uses
`48000 Hz`.

`Approximator` starts from the standard seven-filter defaults in
`StateValueDefinitions.EqualizerDefinitions`. NLopt COBYLA constrains every
parameter to its physical range and minimizes squared response error with an
additional penalty for large individual deviations. The Commit path uses its
dedicated optimization profile: `10000` evaluations and a `1.0e-9` function
tolerance. When committing a source bank other than bank `0`, the target curve
includes both the existing bank `0` filters and the source bank filters. The
resulting approximation replaces bank `0`; committing bank `0` does not include
it twice.

The command frame is:

```text
commit_bank targetInstanceId sourceBankIndex local|group
```

`sourceBankIndex` is zero-based. `local` processes only the addressed instance.
`group` uses the standard instance-control target resolver for the exact group
containing the addressed bank and independently approximates each member's own
source bank into that instance's bank `0`. An ungrouped bank falls back to its
local instance.

All approximations must finish before state mutation begins. The resulting bank
`0` filters are enabled and written in one shared history transaction. When the
source is not bank `0`, its filter parameters and bypass values are reset to
their defaults in the same transaction. Group membership is unchanged. If any
target is missing, the addressed source is empty, or any curve cannot be
approximated, no target is mutated. An empty peer in a valid group contributes
a flat curve and therefore resolves to the standard bank `1` defaults.

The operation runs on the Managed control path. It does not execute work in the
native audio callback and does not add an ABI entrypoint.

The Bank Manager also exposes a `Refine` action beside `Commit`. Refine uses a
beam search with six candidates per level and at most seven selected filters.
Each structural candidate gets its own COBYLA optimization over all of its
continuous parameters, so adding a filter can also change the parameters of
previous filters. Intermediate beam expansions use the fast optimization
profile: `1000` evaluations and a `1.0e-4` function tolerance. Its objective
uses 128 evenly distributed frequency points during candidate optimization; candidate
responses remain available on the full curve for ranking and diversity. The
response loss is weighted by intervals on a logarithmic frequency axis, rather
than by the accidental density of the input samples.

Candidate ranking combines response error with penalties for model complexity,
weak gains, extreme normalized parameters, overlapping responses, and pairs
whose combined response nearly cancels. Structural candidates are canonicalized
by sorted filter indices. The beam is also pruned by response-curve diversity,
so near-identical responses do not consume all beam slots. A candidate must
provide a meaningful marginal improvement to become the global best, while up
to three non-improving depths are still explored for continuations that need an
intermediate structural step. A candidate whose response loss is effectively
zero terminates the structural search immediately, because additional filters
cannot improve an already exact response.

The two best final beam candidates use the balanced profile: `2000` evaluations
and a `1.0e-5` function tolerance on 256 points. Simplification uses the fast
profile (`1000` evaluations on 128 points) because it evaluates many possible
removals. The precise profile remains available for a future quality pass, but
is not used by the latency-oriented Refine path. Commit retains its separate
full-curve profile.
The best result then goes through iterative simplification: each element is
removed in turn, the remaining model is re-optimized, and the removal is kept
when the penalized score does not materially worsen. The selected result is
then applied atomically back to the bank selected by the Refine command. Refine
does not merge another bank, reset the source bank, or reinterpret the selected
zero-based bank index as bank `0`.

The search implementation is split into four Managed services. The generic
`BeamSearch` contains only search control: expansion levels, beam retention,
global-best tracking, and delayed stopping. `EqualizerRefinement` is the
EQ-specific Refine service: it owns candidate expansion, scoring, response
diversity, finalization, simplification, and result decoding. It uses
`RefinementOptimizer` for candidate-specific NLopt optimization and response
loss calculation. `Approximator` is independent and serves only the full
seven-filter Commit approximation.

The Bank Manager exposes `Commit` in its action column. It is enabled when the
focused bank reports an active filter with non-zero gain and sends the current
editing scope.
