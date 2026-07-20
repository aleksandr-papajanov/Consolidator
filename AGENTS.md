# Consolidator Project Rules

## Project

Consolidator is an Ableton Live / Max for Live device that compares a current
signal with a reference, fits an EQ, and processes audio through a stack of EQ
banks.

- `Max/` owns the device patch, feature patchers, JavaScript UI, presentation
  configuration, and built `.mxe64` files.
- `Native/Consolidator/` owns the C++20 Min-DevKit projects.
- `Docs/GeneralRefactor.md`, `Docs/GeneralRefactorPlan.md`, and
  `Docs/AtomProtocol.md` describe the active architecture and protocol.

## Ownership

`DeviceHost` is the sole owner of official runtime state. `EqStore` owns EQ
banks, selected bank, absolute filter values, bypass state, stable IDs, and the
store revision. Components never mutate each other directly.

The normal control flow is:

```text
UI or component -> typed atom command -> DeviceHost -> EqStore commit
DeviceHost -> event and immutable snapshot -> consumers
```

Host transactions collect events while state is locked and dispatch them only
after releasing the lock. Max `send`/`receive` delivery is synchronous, so no
external outlet callback may run while a Host store mutex is held.

The audio and analysis flows are:

```text
audio -> EqChain -> audio
pre-EQ current + reference -> Analyzer -> SpectrumView
Analyzer difference stream -> Approximator -> fit.complete -> DeviceHost
```

Runtime Dictionaries, shared mutable state dictionaries, generations, target
routing, and envelope payload objects are not part of the architecture.

## Native Layers

- `Consolidator.Domain` contains IDs, definitions, states, snapshots, commands,
  events, operation state, and fit results. It has no Max or DSP dependency.
- `Consolidator.Messaging` contains `AtomValue`, readers/writers, message framing,
  and typed command/event/snapshot codecs. It has no Max dependency.
- `Consolidator.DeviceHost` contains `DeviceHost`, transactional stores, and
  workflow coordination. It has no Max dependency.
- `Consolidator.Persistence` contains the typed persistence schema and codec.
- `Consolidator.DspCore` contains snapshot builders and reusable DSP-facing
  projections.
- `Consolidator.Shared` contains transport-neutral audio, curve, EQ, FFT, DSP,
  settings, numeric helpers, and the remaining shared value types.
- `Consolidator.MaxAdapter` is the only native Max transport boundary. It maps
  Min atoms to `AtomList` and Max Dictionaries to persistence objects.

Shared stateful DSP devices must have separate left and right instances.
EqChain builds a replacement stereo chain off the audio thread and swaps it
through `DspCore/RealtimeSnapshotSwap`. The single audio reader uses a hazard
pointer; reference counting, allocation, destruction, locks, and Dictionary
work are forbidden in the sample callback. Analyzer, EqChain, Approximator,
and visual curves use the same
filter formulas, frequency grid, sample-rate defaults, and typed definitions.

## Atom Protocol

All non-audio runtime communication uses one of these atom families:

```text
command <version> <source> <requestId> <name> <fields...>
event <version> host <eventId> <name> <fields...>
snapshot <version> host <store> <revision> <fields...>
```

Commands always go to Host. Events and snapshots always come from Host. There
is no `target` field. Entity IDs are typed fields. Every variable-length array
has an explicit count. Max may deliver a complete atom sequence as `list`; that
is the same protocol message, not a fallback format.

Supported commands are:

- `component.attach <componentId> <type>`
- `component.detach <componentId>`
- `eq.set_parameter <bankId> <filterId> <parameter> <absoluteValue>`
- `eq.set_bypass <bankId> <filterId> <0|1>`
- `eq.reset_filter <bankId> <filterId>`
- `eq.add_bank [name]`
- `eq.remove_bank <bankId>`
- `eq.rename_bank <bankId> <name>`
- `eq.select_bank <bankId>`
- `analyzer.listen <0|1>`
- `fit.start`, `fit.cancel <sessionId>`, and `fit.clear`
- `fit.complete <sessionId> <bankId> <loss> <filterCount> ...`
- `fit.fail <sessionId> <error>`

Every command inlet and outlet must document its complete accepted or produced
command list. Update the Min descriptions and JS `assist()` callbacks whenever
a contract changes.

## State Rules

- Bank IDs and filter IDs are one-based everywhere.
- Bank IDs are stable and never reused after removal.
- Banks are stored and processed in ascending ID order. The UI may display
  them in reverse order but must retain the real ID on each row.
- A successful transaction increments the store revision exactly once.
- Rejected and unchanged operations do not increment the revision.
- Fit applies all returned filter values atomically to the bank captured when
  the fit started.
- Runtime state, snapshots, persistence, graph edits, and DSP use absolute
  values. Normalized `0..1` values exist only between Max controls and the
  Filter UI controller.
- `FilterOptions` is the only source of filter types, parameter names, ranges,
  scales, defaults, and bypass defaults.
- `Max/Config/ConsolidatorSettings.json` contains presentation only: control
  positions, type layouts, visibility/enabled state, and per-slot colors.

## Component Responsibilities

`consolidator.devicehost` receives typed commands, publishes Host events, EQ and
definition snapshots, and owns the private persistence Dictionary boundary.
It must not perform DSP or UI work.

`consolidator.filter.js` is a stateless endpoint for one filter slot. It sends
absolute Host commands and projects selected-bank snapshots to direct local
status. `consolidator.filter.controller.js` owns configuration loading, layout,
normalization, and Max control updates. Controls update only from endpoint
status, using `set` to avoid feedback.

`consolidator.eqchain` receives complete EQ snapshots and processes every bank
in ascending ID order. It knows nothing about selection, UI, persistence,
Analyzer, or Approximator.

`consolidator.analyzer` receives pre-EQ current stereo and reference stereo.
It publishes current, reference, difference, selected-bank filter curves, and
the total EQ response. It derives selected-prefix and total responses from the
same EQ snapshot used by EqChain. FFT delivery uses a Min `queue<>`; do not poll
Analyzer with `qmetro`. Analyzer sends `curve_settings <minimumHz> <maximumHz>
<pointCount>` on its visual outlet; SpectrumView must use that metadata instead
of duplicating the native curve grid.

`consolidator.approximator` receives EQ snapshots and the live difference
stream directly from Analyzer through the scoped Max connection. The stream
does not pass through Host. Host starts a fit with an operation event.
Approximator returns `fit.complete` or `fit.fail`; only Host may commit the
result. A native bounded/SPSC transport is still a future optimization and
must not be assumed by current code.

SpectrumView receives, in order: current spectrum, reference spectrum,
difference, selected-bank filter curves, total EQ response, and Host snapshots.
Marker drag emits absolute `eq.set_parameter` commands. Holding Alt changes Q.

## Max Feature Layout

`Max/Consolidator.amxd` is the root device. Runtime code lives under
`Max/Features/<Feature>/`. A feature owns its patcher, root executable JS or
native external, controller, feature-specific JS, and assets.

Every feature connects internally to the scoped buses:

```text
s ---message.bus.in
r ---message.bus.out
```

BusHub owns the single `consolidator.devicehost` instance. Persistence uses the
separate scoped `---device.persistence.in/out` transport and never enters the
runtime atom bus. The `---` prefix is mandatory so Live scopes names per device
instance.

Max boxes reference runtime dependencies by unique filename, not repository
paths. Native externals are copied directly into their owning feature folder.
Do not create a central `Max/Externals` directory.

Executable JS and `jsui` files instantiated by patchers live in the feature
root and follow `consolidator.<feature>.<role>.js`. Helpers stay in the feature's
`JS/` folder. Painters are self-contained and do not use `include`.

Controllers own local UI behavior only. They do not own domain state, DSP,
persistence, bank models, or interfeature routing. JavaScript domain helpers use
classes with intent-revealing methods; do not build new files as collections of
unrelated global functions.

## Persistence

Persistence is the only Max Dictionary use in native runtime. The root device's
`pattrstorage` sends recalled dictionaries through EqStorage's private third
port to `---device.persistence.in`. DeviceHost validates the schema and complete
EQ invariants before replacing state. Incompatible schemas reset to typed
defaults; no migration or legacy reader is added during development.

The root `pattr eqStorageState` uses `@thru 2`. Host persistence commits arrive
through the pattr inlet and must update storage without being emitted back as a
restore. A `pattrstorage recall` is an external change and must still reach
DeviceHost. Never create a persistence write-to-restore feedback loop.

DeviceHost publishes persistence only after `persistence_ready`. The temporary
Dictionary must travel synchronously through EqStorage's private third outlet
to the root `pattrstorage`; never delay a Dictionary name after its owner has
gone out of scope. Debounce, if introduced, must happen before serialization.
Never release a parent Max Dictionary after appending atoms or a nested
dictionary. A nested dictionary transfers ownership to its parent after a
successful append.

## Code Style

- Use PascalCase for classes, structs, enums, and methods.
- Use camelCase for variables, parameters, fields, and local functions.
- Max-required global callbacks and selectors (`paint`, `list`, `onclick`,
  `filter_curve`, and similar) keep the exact API spelling; they immediately
  delegate to PascalCase class methods.
- Prefer one responsibility per class and intent-revealing file names.
- Keep patchers focused on routing and presentation; keep numerical behavior in
  C++.
- Keep comments minimal and explain only non-obvious intent or ownership.
- Use ASCII for new source and documentation unless a file requires otherwise.
- Do not add compatibility readers, aliases, dual protocols, migrations, or
  fallback message formats. Replace old contracts completely.
- When a local, behavior-preserving rule violation is discovered while reading
  code, fix it as part of the task.

## Verification

For normal implementation work:

- validate changed `.maxpat` and JSON files;
- run `git diff --check`;
- verify every command name, field order, inlet, outlet, and patchcord;
- build affected externals after protocol or shared-header changes;
- run focused tests, then broader tests for shared state or DSP changes.

When the user explicitly postpones builds or tests, perform the static checks
and report that native binaries were not rebuilt.
