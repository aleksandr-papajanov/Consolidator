# Consolidator Project Rules

## Project

Consolidator is an Ableton Live / Max for Live device that compares a current
signal with a reference, fits an EQ, and processes audio through a fixed stack
of EQ banks.

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
audio -> input gain -> saturator -> compressor -> EQ banks -> output gain -> audio
signal after compressor / before EQ + reference -> Analyzer -> SpectrumView
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
  optimization primitives, settings, numeric helpers, and the remaining shared
  value types. Generic search infrastructure must remain independent of EQ,
  DSP, Max, and persistence.
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

`Shared/Workflows/LatestWorkflowExecutor` is the sole generic background
workflow primitive. It owns one worker, retains only the newest immutable task,
and drops cancelled or stale completions. Feature code supplies pure work and a
main-thread completion notifier; workers must never access Max, Live API,
Dictionaries, outlets, or mutable Host stores. Only the main thread applies a
workflow result atomically. Use it for fitting, persistence decoding, and DSP
topology construction. Continuous control gestures never enter a workflow.

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

High-rate parameter commands publish only the short Host event
`parameter.updated <revision> <device> <bankId> <filterId> <parameter> <absoluteValue>`.
`StateTransport` routes it solely to the scoped DSP and Analyzer parameter
channels. It never reaches the runtime event bus, UI controllers, BankManager,
or persistence. Local controllers provide bounded live-link transport, while a
debounced EQ or processor snapshot confirms canonical state after a gesture.
Snapshots never generate link edits.
Discrete bypass and reset operations use explicit bounded link messages; fit,
restore, Join, and other local transactions are never inferred as gestures.

Supported commands are:

- `eq.set_parameter <bankId> <filterId> <parameter> <absoluteValue>`
- `eq.set_parameter_index <bankId> <filterId> <parameterIndex> <absoluteValue>` is internal link transport; UI must use named parameters.
- `eq.set_bypass <bankId> <filterId> <0|1>`
- `eq.reset_filter <bankId> <filterId>`
- `eq.set_chain_bypass <0|1>` and `eq.set_chain_solo <0|1>`
- `eq.reset <bankId>`
- `eq.join_banks <count> <bankIds...>`
- `eq.commit_hidden <bankId>`
- `eq.set_link <bankId> <linkId|->`
- `eq.select_bank <bankId>`
- `gain.set_parameter <input|output> <absoluteGainDb>`
- `compressor.set_parameter <attack|release|threshold|output|mix> <absoluteValue>`
- `compressor.set_detector_parameter <1|2> <gain|frequency|q|bypass> <absoluteValue>`
- `compressor.set_detector_listen <0|1|2>`
- `compressor.set_bypass <0|1>`
- `compressor.reset`
- `saturator.set_parameter <saturation|output> <absoluteValue>`
- `saturator.set_detector_parameter <1|2> <gain|frequency|q|bypass> <absoluteValue>`
- `saturator.set_detector_listen <0|1|2>`
- `saturator.set_bypass <0|1>`
- `saturator.reset`
- `analyzer.clear`
- `analyzer.set_view <0|1> <spectrum|analysis>`
- `fit.start <pointCount> <curveDb...>`, `fit.cancel <sessionId>`, and `fit.clear`
- `fit.complete <sessionId> <bankId> <loss> <filterCount> <filters...> <inputGain> <compressorBypass> <attack> <release> <threshold> <output> <saturatorBypass> <saturation> <saturatorOutput> <outputGain>`
- `fit.fail <sessionId> <error>`

Every command inlet and outlet must document its complete accepted or produced
command list. Update the Min descriptions and JS `assist()` callbacks whenever
a contract changes.

## State Rules

- EQ has exactly seven fixed banks: hidden system bank `0` and user banks
  `1..6`. User-bank and filter IDs are one-based everywhere. Bank `0` is never
  manually edited or linked.
- User banks contain only filter state and an optional `linkId`; names and
  per-bank bypass/solo state do not exist. EQ chain `bypass` and `solo` are
  operational state on `EqSnapshot`, independent of bank selection. `solo`
  auditions the selected user bank only.
  - Without Solo, DSP and the total EQ response apply every bank in ascending
    ID order, including hidden bank `0`. Selection never changes the normal
    DSP chain.
  All user banks are part of the normal DSP stack.
- A successful transaction increments the store revision exactly once.
- Rejected and unchanged operations do not increment the revision.
- Restore assigns every store a revision newer than the currently published
  state; restore snapshots must never move revisions backwards.
- Fit applies all returned filter values atomically to the bank captured when
  the fit started.
- Runtime state, snapshots, persistence, graph edits, and DSP use absolute
  values. Normalized `0..1` values exist only between Max controls and the
  Filter UI controller.
- `FilterOptions` is the only source of filter types, parameter names, ranges,
  scales, defaults, and bypass defaults. All EQ filters belong to one ordered
  chain; placement is not part of filter state or definitions. Shelf and tilt
  Q values are fixed DSP settings and are not exposed as filter parameters.
- `CompressorOptions` and `SaturatorOptions` are the only sources of processor
  ranges and defaults, including mix and detector-filter state.
  `GainOptions` owns input/output gain range and default. Compressor ratio and
  the compressor ratio is fixed in `CompressorOptions`.
- `Max/Config/ConsolidatorSettings.json` contains presentation-only per-filter
  colors. Runtime parameter definitions and ranges never come from JSON.

## Component Responsibilities

`consolidator.devicehost` receives typed commands, publishes Host events, EQ and
definition snapshots, and owns the private persistence Dictionary boundary.
Store events are immediate. A continuous parameter commit emits only
`parameter.updated`; DSP updates the matching existing stereo device target and
Analyzer updates its local EQ visual model. It must not build or publish an EQ,
processor, or DSP snapshot. After the restartable 100 ms persistence debounce,
Host serializes persistence and publishes only the compact EQ or processor
confirmation required to refresh canonical UI state. Structural changes publish
the appropriate snapshots immediately. DeviceHost must not perform DSP or UI
work. Dictionary conversion is main-thread-only; typed persistence decoding runs
through `LatestWorkflowExecutor`, then `DeviceHost::Restore` and publication run
on the main thread.
`eq.select_bank` is DSP-independent unless EQ Solo is active; `eq.set_link`
is always DSP-independent. Both publish EQ state for UI and Analyzer without a
complete DSP snapshot.
Static features do not register or perform startup handshakes. After the root
device's deferred persistence restore, one `persistence_ready` message makes
Host publish definitions, EQ, processor, and DSP snapshots exactly once.

`consolidator.dspprocessor` receives complete DSP snapshots only for startup,
restore, and topology changes. It receives `parameter.updated` directly for a
continuous target update and updates only the matching existing left/right DSP
device, without rebuilding the chain. It builds the full chain in this fixed
order: input gain, saturator, compressor, all EQ filters from
every bank in ascending bank/filter ID order, then output gain. It knows nothing about selection, UI, persistence, Analyzer,
or Approximator. It publishes bounded latest-value processor telemetry directly
to Analyzer over the scoped `---processor.telemetry` transport. Telemetry
contains measured compressor gain reduction, saturator nonlinear residual,
saturator level delta, RMS levels before/after both gain stages, and measured
compressor/saturator output RMS; it never enters Host or the runtime atom bus.
Its first two signal outlets are the final stereo output; outlets three and four
are the stereo EQ-input tap after compressor and before the EQ banks. The
`DspProcessor.maxpat` feature must pass all four signal outlets through in that
same order before routing status, diagnostics, and telemetry.
Topology construction is a latest-wins background workflow. Only a completed
stereo chain is installed on the main thread; normal parameter target updates
continue to update existing devices directly and never wait for topology work.

`consolidator.analyzer` receives both the final post-EQ current signal and the
DSP tap after compressor and before the EQ banks, plus the unprocessed reference stereo.
It publishes measured post-EQ current, reference and
difference curves, selected-bank filter curves, the total EQ response, and a
rolling feature vector with global and standard-band metrics. It never adds a
calculated EQ response to the measured current. Fit accumulation compares the
pre-EQ tap with the reference and subtracts the ordered bank response. The audio thread owns smoothing state and
accumulates the smoothed difference curve as a running mean whenever a reference
signal is present. `analyzer.clear` clears that accumulation without disabling
subsequent analysis.
The audio thread
publishes immutable `AnalyzerCurveFrame` values through a preallocated
single-producer/single-consumer triple buffer. Its overflow policy is
latest-wins and replaced frames are counted. One coalesced Min `queue<>` handoff
delivers the newest frame on Max's main thread; do not poll Analyzer with
`qmetro`. Analyzer sends `curve_settings <minimumHz> <maximumHz> <pointCount>`
on its visual outlet; SpectrumView must use that metadata instead of
duplicating the native curve grid.

`analyzer.set_view` is operational, non-persistent view demand. When the
device's owning track is not the selected Live track, Analyzer view demand is
disabled. In `spectrum`
mode Analyzer produces FFT curves only; in `analysis` mode it produces the
feature vector only. Reference-driven fit-curve accumulation remains active
while the Analyzer view is hidden. A
silent FFT window skips FFT post-processing, metric extraction, curve
serialization, and Max delivery. Processor telemetry likewise suppresses silent
windows before it reaches Max. Stateful audio DSP is never skipped merely
because its input is silent.
Analyzer view demand is published only after the native Analyzer has accepted
the first canonical EQ snapshot and emitted `status host_ready`. The controller
may resolve Live track selection earlier, but it retains that state until Host
and Analyzer readiness are confirmed instead of relying on startup timing.

`consolidator.approximator` owns the EQ curve workflow only. Its UI controller
caches the latest Analyzer `fit_curve` and sends it atomically in
`fit.start <pointCount> <curveDb...>`. DeviceHost publishes one internal
`fit.requested <sessionId> <bankId> <pointCount> <curveDb...>` event, and the
Approximator returns one `fit.complete` or `fit.fail`. It has no audio signal
inlet, capture buffer, or offline dynamics/saturation workflow. `fit_curve` is
the accumulated Analyzer difference at the EQ input minus the response of the
ordered banks `1..selected`. `analyzer.clear` resets that Analyzer accumulation
and is independent of the Approximator. Analyzer publishes only
the tagged `fit_curve` message through `---analyzer.curves`. Beam Search remains
dormant infrastructure for future structural search. Only Host commits a fit
result atomically.

`BankManager` is the single bank UI. It renders the six local user banks and
the bank summaries of other registered instances. A normal local click selects
the active bank. Ctrl/Cmd maintains a separate Link selection and never changes
the active bank. `Join` copies audible filters from the selected local user
banks into hidden bank `0` and clears those source banks in one Store commit.
`Commit` is available only for an empty user bank: Fit approximates hidden bank
`0` into that user bank and clears bank `0` only after a successful atomic Host
commit. Link selection accepts occupied or empty user banks; each bank belongs
to at most one Link group, and a group contains at most one bank per device
instance.

`link.join <linkId> <sourceDeviceId> <revision>` is a group command on the
global host bus. Each participant joins its occupied bank(s) with that link into
its own hidden bank `0`; the link remains intact. The resulting default source
values are local Join effects and must never be replicated as parameter edits.

A bank `linkId` is the only link-group identity. It links that bank's EQ filters
together with the instance input gain, compressor, saturator, and output gain.
There is no separate processor link state or processor link command. The
selected local bank chooses the active processor-link context; selecting an
unlinked bank restores default control colors and ranges.

Linked continuous edits use deltas in each parameter definition's normalized
coordinate so linear and logarithmic controls preserve the same relative
gesture. EQ uses
`link.filter_delta <linkId> <sourceRuntimeId> <revision> <filterId> <parameterIndex> <normalizedDelta>`
and processors use
`link.processor_delta <linkId> <sourceRuntimeId> <revision> <device> <parameter> <normalizedDelta>`.
Only the global link transport uses normalized deltas; all Host commands,
snapshots, stores, and DSP values remain absolute.
BankManager computes hard control limits from all group members when the active
link or membership changes and sends them over the scoped
`---link.control.state` transport. It does not recalculate limits during a
gesture. Remote deltas update the cached model of every group member before the
local Host command is emitted, and they are never broadcast again.

Analyzer visualization is split into independent responsive JSUI components.
`consolidator.analyzer.view.js` is the single responsive JSUI for Analyzer
visualization. It switches between spectrum and analysis pages without creating
parallel canvases. It receives current spectrum, reference spectrum, fit curve,
selected-bank filter curves, total EQ response, feature vectors, and Host
snapshots; marker drag emits absolute `eq.set_parameter` commands and Alt changes Q.
`consolidator.analyzer.processormeters.js` receives processor telemetry only.
`consolidator.analyzer.gainmeters.js` receives Analyzer `gain_levels` only.
The four boxes must remain independently resizable and composable in Max
presentation; no component derives its drawing area from a sibling box.

Analyzer JSUI only renders prepared values. Temporal smoothing, FFT sampling,
curve aggregation, and metric extraction belong to C++; JS must not use
gradients, curve interpolation, or per-frame data transformations for these
views. Keep the root JSUI small and split state, geometry, rendering, and input
into feature-local helper files.
EQ snapshot-driven Analyzer filter visuals must coalesce on Max's main thread;
do not synchronously rebuild and publish every filter curve for each parameter
event. BankManager announcements are summary changes only. Continuous linked
parameter traffic must not rebroadcast announcements or redraw every manager.

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

`---message.bus.in` carries typed commands to Host. `---message.bus.out`
carries small Host events only. Full state never enters either runtime bus.
`StateTransport` routes immutable Host snapshots to scoped, store-specific
latest-state channels:

```text
---state.eq
---state.dsp
---state.processor
---state.definitions
---state.device
---state.analyzer
---dsp.parameter
```

Each feature subscribes only to the stores it consumes. `---dsp.parameter`
contains only `parameter.updated` events and is consumed by DspProcessor;
`---state.analyzer` receives the same short event only so native Analyzer can
update its visual EQ model. Do not add a generic
snapshot receiver to a controller or reconnect state channels to the runtime
event bus. `---state.dsp` is the complete chain state and is consumed only by
DspProcessor. Approximator combines `---state.eq` and the compact
`---state.processor` snapshot. UI controllers and BankManager also consume the
compact processor state. `---state.analyzer` contains
only Analyzer events and EQ snapshots needed by the native Analyzer and
SpectrumView.

Cross-device coordination uses the unscoped `consolidator.host.bus` transport;
it is intentionally distinct from the per-device `---message.bus.*` runtime
bus. `BankManager` is its consumer and producer. Every instance announces a
runtime ID derived from its Live device object, its label, active user bank,
bank occupancy and link memberships. The runtime ID must never come from
persisted state: copying a device duplicates persistence but must create a
separate peer row. A peer announcement updates only that peer row and must not
cause a Live-set traversal. `bank.announce` contains topology summaries only;
it never carries filter or processor snapshots. Linked participants initialize
their peer models with one bounded `link.state` frame per linked bank only
after startup, an explicit `bank.query`, or a changed link membership. Bank
selection and ordinary snapshot confirmation publish only `bank.announce`,
never a full linked-state replay. Link updates carry
`linkId`, source runtime ID, and a monotonically increasing revision. A
received remote update must apply in remote mode and never be broadcast again.
Revisions are monotonic per source runtime ID and link; receivers track them by
`(linkId, sourceRuntimeId)`. A single revision counter shared by all members can
reject valid edits from another participant.

Continuous linked-control gestures use the scoped
`---link.parameter.gesture` transport from local EQ, gain, compressor, and
saturator controllers to `BankManager`. Controllers publish normalized
`eq_parameter_gesture` or `processor_parameter_gesture` values before the
corresponding absolute Host command. Continuous controller updates use a
latest-value dispatcher capped at one update per parameter every 16 ms; stale
intermediate values must never accumulate in the Max main-thread queue.
SpectrumView marker edits use the same dispatcher and publish
`eq_parameter_absolute_gesture`; BankManager resolves the named absolute value
through the typed filter definition before broadcasting the normalized delta.
`BankManager` broadcasts the relative delta immediately. The receiving local
Host commits the absolute target and emits a short `parameter.updated` event;
the delayed canonical snapshot must not rebroadcast that delta.

BankManager keeps the current absolute EQ and processor values only for linked
participants. Refresh compact peer topology and linked state synchronously
before creating a group, then keep those models current with every accepted relative delta.
Continuous values travel only through delta messages; `bank.announce` must not
duplicate the same edit. The gesture path must not rescan group members or
recompute limits. It validates message shape and publishes the delta once;
the fixed control limits established by `link.state` constrain the gesture.
Dial and slider controls enforce the fixed limits during the gesture; rollback
is not a second transport path. Compressor deltas include attack,
release, threshold, output, and mix; saturator deltas include saturation and output; gain
deltas include gain.

Linked gestures have a separate scoped preview lane on
`---link.control.state`: `eq_preview <bankId> <filterId> <parameterIndex>
<absoluteValue>` and `processor_preview <device> <parameter> <absoluteValue>`.
`BankManager` emits a preview immediately after updating its local linked model,
both for source and remote deltas. UI controllers apply it directly without
creating a Host command. A Host `parameter.updated` event updates DSP and
Analyzer immediately; the delayed snapshot is canonical confirmation and must
never be required for a remote linked control to visually track a gesture.

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

JavaScript features that require Live API identity use the shared idempotent
`LiveApiInitializer`. They start it from `loadbang`, retry only until Live API
becomes available, and stop permanently after success. Correct initialization
must not depend on a single external `live.thisdevice -> deferlow` bang because
Live can load devices on non-selected tracks lazily. Any retained external
`initialize` message is an optional idempotent trigger, not the lifecycle owner.

## Persistence

Persistence is the only Max Dictionary use in native runtime. Schema 15 stores
the fixed EQ banks, stable `instanceId`, input/output gain, compressor, and saturator state. The root device's
`pattrstorage` sends recalled dictionaries directly to `---device.persistence.in`.
DeviceHost validates the schema and complete
EQ invariants before replacing state. Incompatible schemas reset to typed
defaults; no migration or legacy reader is added during development.

The root `pattr eqStorageState` uses `@thru 2`. Host persistence commits arrive
through the pattr inlet and must update storage without being emitted back as a
restore. A `pattrstorage recall` is an external change and must still reach
DeviceHost. Never create a persistence write-to-restore feedback loop.

The root `pattrstorage` is the parameter-enabled Blob persistence boundary. It
must retain `parameter_initial_enable` and `paraminitmode`; Blob visibility is
not changed to implement persistence. After `persistence_ready`, DeviceHost
synchronously publishes canonical state so defaults or restored state are
written to slot `1` before Live serializes it.
The source AMXD contains an empty factory Blob and the `pattr` object contains
no embedded runtime `restore` value. If no valid per-instance Blob is recalled,
DeviceHost creates typed defaults before publishing ready. Never save a test
instance's runtime Dictionary into the source AMXD because every newly inserted
device would inherit its parameters and link IDs.

DeviceHost publishes persistence only after `persistence_ready`. The temporary
Dictionary must travel synchronously through `---device.persistence.out` to the
root `pattrstorage`; never delay a Dictionary name after its owner has
gone out of scope. Persistence debounce happens before serialization; the
timer callback creates and sends the temporary Dictionary synchronously.
Bank link membership is structural state: successful `eq.set_link` commands
publish persistence immediately instead of waiting for the normal parameter
debounce.
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

Use `.vscode/build-all.cmd` for native builds. Its runtime configuration is
`RelWithDebInfo`; never copy Debug externals into the Max device for profiling
or normal Live use.

Native builds must be started only through the VS Code default task
`Consolidator: Build All`, which executes `.vscode/build-all.cmd`. Do not invoke
CMake or Visual Studio build commands directly.
