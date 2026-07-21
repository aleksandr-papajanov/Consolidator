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
banks and filters, `CompressorStore` owns compressor state, and
`SaturatorStore` owns saturator state. Each store has one domain responsibility;
do not combine unrelated devices into a generic processor store. Components
never mutate each other directly.

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
audio -> DspProcessor -> audio and Analyzer candidate input
post-DSP candidate + reference -> Analyzer -> SpectrumView
Fit capture (pre-DSP current + reference) -> offline Approximator -> fit.complete -> DeviceHost
```

Runtime Dictionaries, shared mutable state dictionaries, generations, target
routing, and envelope payload objects are not part of the architecture.

## Native Layers

- `Consolidator.Domain` contains IDs, definitions, states, snapshots, commands,
  events, operation state, and fit results. It has no Max or DSP dependency.
- `Consolidator.Messaging` contains `AtomValue`, readers/writers, message framing,
  and typed command/event/snapshot codecs. It has no Max dependency.
- `Consolidator.DeviceHost` contains `DeviceHost`, transactional stores, and
  workflow coordination. `AnalyzerWorkflow` owns analyzer listen sessions and
  `FitWorkflow` owns fit transitions, the captured target bank, result
  validation, and the single complete-chain fit apply. `DeviceHost` only routes typed
  commands, serializes access, and dispatches events. It has no Max dependency.
- `Consolidator.Persistence` contains the typed persistence schema and codec.
- `Consolidator.DspCore` contains snapshot builders and reusable DSP-facing
  projections.
- `Consolidator.Shared` contains transport-neutral audio, curve, EQ, FFT, DSP,
  settings, numeric helpers, and the remaining shared value types.
- `Consolidator.MaxAdapter` is the only native Max transport boundary. It maps
  Min atoms to `AtomList` and Max Dictionaries to persistence objects.

Shared stateful DSP devices must have separate left and right instances.
DspProcessor builds a stereo chain off the audio thread and keeps it alive while
its topology is unchanged. Snapshot updates with the same device ID, type, and
order update only atomic parameter targets on the existing left/right devices;
device state is preserved and each device smooths its own parameters using
`AudioOptions`. `DspCore/RealtimeSnapshotSwap` is used only when topology or
sample rate changes. Allocation, destruction, locks, reference counting, and
Dictionary work are forbidden in the sample callback. Analyzer, DspProcessor, Approximator,
and visual curves use the same
filter formulas, frequency grid, sample-rate defaults, and typed definitions.
Every `DspDeviceRegistration` has an explicit `order`; `DspChainBuilder` sorts
registrations by it before building either channel. Runtime assembly assigns
input gain first, EQ filters in bank/filter order, compressor, saturator, and
output gain. Execution order is topology metadata and is not persisted in EQ
state.

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

- `eq.set_parameter <bankId> <filterId> <parameter> <absoluteValue>`
- `eq.set_bypass <bankId> <filterId> <0|1>`
- `eq.reset_filter <bankId> <filterId>`
- `eq.set_section_bypass <bankId> <pre|post> <0|1>`
- `eq.reset_section <bankId> <pre|post>`
- `eq.add_bank [name]`
- `eq.remove_bank <bankId>`
- `eq.rename_bank <bankId> <name>`
- `eq.select_bank <bankId>`
- `gain.set_parameter <input|output> <absoluteGainDb>`
- `compressor.set_parameter <attack|release|threshold> <absoluteValue>`
- `compressor.set_bypass <0|1>`
- `compressor.reset`
- `saturator.set_parameter <absoluteSaturation>`
- `saturator.set_bypass <0|1>`
- `saturator.reset`
- `analyzer.listen <0|1>`
- `fit.start`, `fit.cancel <sessionId>`, and `fit.clear`
- `fit.complete <sessionId> <bankId> <loss> <filterCount> <filters...> <inputGain> <compressorBypass> <attack> <release> <threshold> <saturatorBypass> <saturation> <outputGain>`
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
- `FilterOptions` is the only source of filter types, PreEq/PostEq placement,
  parameter names, ranges, scales, defaults, and bypass defaults. PreEq contains
  tilt 2 and bells 4-5. PostEq contains shelves 3 and 6 and bells 7-9.
- `CompressorOptions` and `SaturatorOptions` are the only sources of processor
  ranges and defaults. `GainOptions` owns input/output gain range and default.
  Compressor ratio is fixed in `CompressorOptions`.
- `Max/Config/ConsolidatorSettings.json` contains presentation-only per-filter
  colors. Runtime parameter definitions and ranges never come from JSON.

## Component Responsibilities

`consolidator.devicehost` receives typed commands, publishes Host events, EQ and
definition snapshots, and owns the private persistence Dictionary boundary.
Store events are immediate. Repeated store commits coalesce into the latest EQ
snapshot on the Max main thread, and persistence preparation uses a restartable
100 ms debounce before serialization. It must not perform DSP or UI work.
Static features do not register or perform startup handshakes. After the root
device's deferred persistence restore, one `persistence_ready` message makes
Host publish definitions, EQ, and DSP snapshots exactly once.

`consolidator.dspprocessor` receives complete DSP snapshots and builds the full
chain in this fixed order: input gain, PreEq filters from every bank in ascending
ID order, compressor, saturator, PostEq filters from every bank in ascending ID
order, then output gain. It knows nothing about selection, UI, persistence, Analyzer,
or Approximator. It publishes bounded latest-value processor telemetry directly
to Analyzer's processor meters over the scoped `---processor.telemetry` transport. Telemetry
contains measured compressor gain reduction, saturator nonlinear residual, and
saturator level delta; it never enters Host or the runtime atom bus.

`consolidator.analyzer` receives the real post-DSP candidate stereo and the
unprocessed reference stereo. It publishes measured current, reference and
difference curves, selected-bank filter curves, the total EQ response, and a
rolling feature vector with global and standard-band metrics. It never adds a
calculated EQ response to the measured candidate. The audio thread owns smoothing state and
publishes immutable `AnalyzerCurveFrame` values through a preallocated
single-producer/single-consumer triple buffer. Its overflow policy is
latest-wins and replaced frames are counted. One coalesced Min `queue<>` handoff
delivers the newest frame on Max's main thread; do not poll Analyzer with
`qmetro`. Analyzer sends `curve_settings <minimumHz> <maximumHz> <pointCount>`
on its visual outlet; SpectrumView must use that metadata instead of
duplicating the native curve grid.

`consolidator.approximator` receives complete DSP snapshots plus scoped pre-DSP
current and reference signals. `fit.start` captures one fixed four-second stereo
fragment into a preallocated buffer. A worker evaluates every candidate against
that immutable fragment with the same DSP chain and feature pipeline used by the
runtime components. The first half-second is processed as deterministic device
warm-up and excluded from loss. NLopt minimizes that loss over normalized,
bounded coordinates which are converted back into absolute snapshot values.
Do not replace NLopt with a hand-written parameter sweep. Optimization covers every parameter of
non-bypassed selected-bank filters, both gains, and every parameter of
non-bypassed compressor and saturator devices. Candidates never touch live DSP,
Host, persistence, or UI. Approximator returns one complete `fit.complete` or
`fit.fail`; only Host commits the final EQ and processor state.

Analyzer visualization is split into independent responsive JSUI components.
`consolidator.analyzer.spectrum.js` receives current spectrum, reference spectrum,
difference, selected-bank filter curves, total EQ response, and Host snapshots;
marker drag emits absolute `eq.set_parameter` commands and Alt changes Q.
`consolidator.analyzer.analysis.js` receives feature vectors only.
`consolidator.analyzer.processormeters.js` receives processor telemetry only.
The three boxes must remain independently resizable and composable in Max
presentation; no component derives its drawing area from a sibling box.

`Max/Features/ProcessorControls/ProcessorControls.maxpat` is the single control
surface for all EQ filters, compressor, and saturator. Max controls are created
and laid out explicitly in the patcher; the controller must never script or
create UI objects. Controls send the normalized messages documented in the
feature README to the controller's local inlet. The controller converts them
to absolute Host commands and returns confirmed normalized values as
`script sendbox <varname> set <value>` commands directly to `thispatcher`.
It owns no official state. Do not recreate per-filter
feature instances or a separate compressor/saturator controller layer.

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

Persistence is the only Max Dictionary use in native runtime. Schema 6 stores
EQ, input/output gain, compressor, and saturator state. The root device's
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
gone out of scope. Persistence debounce happens before serialization; the
timer callback creates and sends the temporary Dictionary synchronously.
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
