# Emulation Lab

`EmulationLab` is the only offline measurement and optimization executable in
the repository. It hosts arbitrary VST3 reference and candidate plugins,
captures versioned datasets, fits exposed candidate parameters with NLopt and
validates the result against a separate dataset. It is not linked into any
realtime target.

## Experiment contract

An experiment manifest defines:

- reference and candidate plugin paths, type queries and optional base states;
- sample rates, block size and deterministic seed;
- named control conditions, including independent reference and candidate
  states or textual parameter assignments;
- probes and objective weights;
- bounded normalized candidate parameters;
- ordered optimization stages with parameter and probe patterns;
- independent training, validation and listening artifacts.

Relative paths are resolved from the manifest directory. A fit rejects unknown
model adapters, empty stages, invalid bounds and incompatible dataset schemas.
The same candidate VST parameter cannot be both a condition control and an
optimizer parameter; that would make one layer silently overwrite the other.
The current model adapter is `vst3-parameters`: model topology remains explicit
and the candidate exposes each tunable algorithm coefficient as a VST3
parameter. This keeps model choice out of the optimizer while allowing the same
workflow to fit unrelated plugins.

## Measurement probes

`linear-sweep` and `harmonic-grid` use settled tones. Every isolated point has
a continuous-tone preroll of at least 12 periods and a measured window of at
least 20 periods. Reported plugin latency is removed before analysis, a Hann
projection measures the fundamental and harmonics above Nyquist are omitted.

Other probes are independent sequences:

- `level-sweep` keeps processor state and oscillator phase across levels;
- `transient-burst` measures a windowed RMS envelope, overshoot and a sampled
  release curve rather than carrier zero crossings;
- `multitone` snaps primaries to coherent bins and measures the residual after
  least-squares sine/cosine projection;
- `stereo-interaction` drives each channel independently and measures channel
  gain and crosstalk.

Every feature has a stable identifier, group, unit, normalization scale,
comparison mode and optional significance floor. Phase uses wrapped angular
error. dB, degrees and milliseconds are normalized before weighting, so their
numeric magnitudes cannot silently determine the objective. Errors are averaged
inside each probe/feature-group pair before manifest weights are applied, so a
group does not become more important merely because it exposes more bins.

## Dataset and reproducibility

Dataset schema version 3 stores the measurement-configuration signature,
observation and sequence identities, condition,
probe version, sample rate, input metadata and named features. The signature is
derived from control conditions, sample rates, block size, seed, probe list and
every probe setting. Loading a legacy or incompatible
schema fails instead of truncating feature vectors or comparing measurements
made with different windows and grids.

Capture writes a sibling environment artifact with plugin identity and content
fingerprint, state fingerprints, conditions, sample rates, block size, latency,
seed and probe list. Run identifiers are unique. Comparison reports retain the
same configuration signature. Reference and candidate datasets must belong to
the same experiment and feature schema.

## Commands

```powershell
cmake -S EmulationLab -B EmulationLab/build
cmake --build EmulationLab/build --config Release

EmulationLab.exe manifest validate Experiments/device/manifest.json
EmulationLab.exe plugin describe Plugin.vst3 "Plugin Name"
EmulationLab.exe plugin save-state Plugin.vst3 state.bin "Plugin Name" "Amount=100"
EmulationLab.exe plugin parameter-manifest Plugin.vst3 parameters.json "Plugin Name"

EmulationLab.exe capture reference manifest.json reference-full.json
EmulationLab.exe split manifest.json reference-full.json
EmulationLab.exe capture candidate manifest.json candidate.json
EmulationLab.exe compare manifest.json reference.json candidate.json report.json
EmulationLab.exe validate manifest.json reference.json candidate.json 1.0
EmulationLab.exe fit manifest.json fit.json
EmulationLab.exe fit manifest.json fit.json previous-fit.json
```

`split` assigns complete stateful sequences, rather than individual observations,
to deterministic training and validation partitions using
`splits.validationFraction`. This prevents a level-transition sequence from
being cut in half and makes the validation grid genuinely held out.

Each stage may set `maximumObservations`; an evenly distributed deterministic
subset is then rendered during search, while zero uses the complete selected
training grid. Stateful probes still render their complete sequence before the
selected observations are scored.

`fit` reads `splits.training`, executes the declared stages, writes a checkpoint
every 25 evaluations and uses the previous stage winner as the next stage base.
After fitting it captures the final candidate and writes a report against
`splits.validation`. Resume files seed parameter values but do not deserialize
NLopt internal populations.

The generic optimizer performs bounded CRS2-LM search, retains a normalized
top-K beam and refines each retained candidate with BOBYQA. It knows only
parameter descriptors and scalar objectives; audio and feature semantics stay
in the VST3 problem adapter and probes.

## Acceptance boundary

Optimization results never rewrite production defaults. A parameter set is
accepted only after held-out validation and listening material agree. Settled
tones alone do not validate dynamics, stereo linking, automation, aliasing or
program dependence; manifests must include the corresponding probes and
conditions when those behaviors matter.
