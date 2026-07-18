# Consolidator Project Rules

## Project

Consolidator is an Ableton Live / Max for Live audio tool for comparing a
current signal with a reference signal, fitting an EQ, and applying the
result through dynamically defined filter objects.

The repository has two layers:

- `Max/` contains Max for Live patchers, UI views, routing, configuration, and
  the built `.mxe64` externals used by Live. Each external is copied into its
  owning feature directory, never into a central `Max/Externals` directory.
- `Native/Consolidator/` contains the C++20 Min-DevKit project. Its Max
  externals are built from `Source/Projects/`.

## Architecture

### Native components

- `Consolidator.Shared` is the transport-neutral native foundation.
  `Audio/AudioBlockView` is a non-owning mono block. `DSP/IDspDevice`
  processes a sample or its default block loop, and `DSP/DspChain` applies an
  ordered set of devices to that block. Stateful stereo processing must own
  separate left and right device instances; never process both channels with
  one stateful chain. `DSP/Spectrum/FftSettings` owns FFT configuration and
  `DSP/Spectrum/FftEngine` is the transport-neutral
  radix-2 real/complex FFT layer; windowing, dB conversion, and display
  smoothing belong above it. `DSP/Curve/`
  contains `ICurveSource`, generic `Curve`, and `CurveRenderer`. `DSP/Eq/IEqFilter`
  combines both for frequency-response filters. `DSP/Eq/Filters/` contains
  `BiquadFilter`, concrete filter settings and filters. `BiquadFilter` owns
  shared state and response behavior; concrete filters own their coefficient
  formulas. `DSP/Eq/Eq` composes EQ filters. `Messaging/` owns envelope and
  factory infrastructure, `Messaging/Messages/` owns typed message contracts,
  and `Application/ComponentRouter` dispatches a component's compile-time
  message list to typed `OnMessage` overloads.
  Only reusable domain
  state such as `FilterDefinition`, `FilterState`, `EqSnapshot`, and the
  complete `DeviceState` belongs in
  `Consolidator.Shared/Models/`; one-off command data stays in message fields.
  `Consolidator.Shared/Settings/GlobalSettings.h` owns shared defaults and
  global constants for this new foundation. Reusable numeric sanitization,
  clamping, range validation, and math utilities belong in
  `Consolidator.Shared/Helpers/`. Keep domain equations and algorithm steps in
  their owning DSP classes; extract only reusable mathematical operations.
  `Consolidator.Shared/Audio/` owns shared audio value types including
  `StereoSample`, `AnalyzerInputFrame`, `AudioFormat`, and `StereoBufferView`.
  `DSP/IDspDeviceFactory`, `DSP/DspChainBuilder`, and `DSP/StereoDspChain`
  build arbitrary ordered mono or stereo device chains without knowledge of
  EQ, Max, or concrete device contracts. Concrete adapters register factories
  for EQ filters, compressors, saturators, or future devices.
- `Consolidator.MaxAdapter` is the only native layer that depends on Max
  dictionaries. `MaxDictionarySerializer` is the single transport boundary and
  exposes `Deserialize<T>`/`Serialize<T>` for registered types. It recursively
  converts Max dictionaries to the transport-neutral `MessageObject`; one
  `DictionaryCodec<T>` specialization maps that object to each model. Do not
  add model-specific Max adapters or repeat atom/dictionary parsing. The
  registered codecs cover `MessageEnvelope` and the complete `DeviceState`,
  including its filter definitions.
  `ComponentHost` is the common native component boundary. It deserializes and
  targets envelopes, loads newer `DeviceState` generations, invokes
  `OnDeviceStateChanged`, and delegates the component's declared message types
  to `ComponentRouter`. `ComponentOutputs` is the matching typed command,
  status, and diagnostic output boundary. Native components must declare their
  accepted message types in `ComponentHost` and implement typed callbacks;
  they must not repeat envelope parsing, target checks, generation checks, or
  selector/dynamic-cast routing.
  When `MaxDictionarySerializer` appends a nested Max dictionary, ownership is
  transferred to the parent dictionary. The nested object must not be released
  or left owned by a `min::dict` wrapper afterward. A serialized root
  dictionary must be sent while its owning scope is still alive; never return
  only its registered name and destroy the dictionary before `outlet.send`.
- `Consolidator.Analyzer` receives current and reference stereo signals and
  publishes spectrum curves and their difference. Curve delivery is scheduled
  on the Max main thread with a Min `queue<>` immediately after an FFT frame is
  completed; do not poll Analyzer with `qmetro` or an `analyzer.publish`
  message. FFT size, spectrum smoothing, calibration, tilt, dB bounds, and
  related numeric analysis defaults come only from
  `Consolidator.Shared/Settings/GlobalSettings.h`; Analyzer exposes no Max
  attributes for them. Disabling `analyzer.difference` resets its difference smoothing;
  the Approximator feature also clears the retained fit curve and the
  Spectrum difference layer so stale data cannot remain ready or visible.
  Analyzer receives `device.state.changed` and reads the complete atomic device state
  from the referenced StateStore dictionary. It derives the selected bank's filter curves, the selected bank
  response, the selected-prefix response, and the total response from that
  single state source.
- `Max/Features/Filter/consolidator.filter.js` is a stateless endpoint for one
  configured filter. It converts local UI activity into `filter.control` and
  forwards `filter.reset`. DeviceStateStore owns
  all absolute values and returns the canonical `filter.state`; the endpoint
  only forwards its normalized control projection as local status. It does not own
  EQ state, process audio, or publish visual curves.
- `Consolidator.EqChain` owns the active filter chain and processes stereo
  audio. It consumes only complete `device.state.changed` state generations;
  it has no selected-bank, UI, analysis, or visualization behavior.
- `Consolidator.Approximator` reads the available filter contracts, selected
  bank, and selected-bank EQ baseline from StateStore, receives the live
  difference curve from Analyzer, and emits absolute
  `filter.set_many` values for the bank captured when fitting started.

### Max data flow

The normal fitting path is:

`StateStore -> Analyzer -> difference/baseline -> Approximator -> StateStore -> EqChain`

The audio path is:

`audio -> EqStorage (EqChain) -> audio`

The visual path is:

`Analyzer -> SpectrumView or FilterCurveView`

### Max feature domain

`Max/Consolidator.amxd` is the root device. Feature code lives under
`Max/Features/<FeatureName>/`; each feature owns its `.maxpat`, its `JS/`
directory, and later any feature-specific images or other assets.
`Max/Features/Shared/` owns reusable infrastructure and abstractions: `JS/`
for shared feature code and `Patches/` for reusable Max patchers. Do not put
feature-specific files in `Max/Features/Shared/`, and do not add new runtime
files to the legacy `Max/JavaScript/` tree.

Max object boxes reference runtime dependencies by their unique filename, not
by a slash-separated repository path. The `.amxd` dependency cache owns each
file's `bootpath`. Keep runtime filenames unique across features so Max can
resolve and cache them without collisions.

Every feature patcher connects to the message bus internally. Outgoing
envelopes feed `s ---message.bus.in`; incoming envelopes come from
`r ---message.bus.out`. BusHub owns `r ---message.bus.in ->
consolidator.bushub.controller.js -> consolidator.bushub.js ->
s ---message.bus.out`. Do not expose or duplicate message-bus ports in the root
device. The `---` prefix is mandatory because Max for Live expands it per
device instance; never replace it with an unscoped global send/receive name.
Native participants that have not yet received a feature wrapper may use one
adjacent scoped send/receive pair in the root until they are migrated.

The first inlet and outlet of every non-DSP native external are reserved for
the same message envelope bus. `Analyzer` and `EqChain` use Min
`sample_operator`, which requires their signal inlets to remain first; their
command-bus inlet follows the signal inlets. The second non-audio outlet is a
standardized direct status outlet:
`status initializing`, `status ready`, `status processing`, or
`error <code>`. Audio, curves, and all specialized data ports follow the two
standard ports. This ordering is intentional and must be applied consistently
when each external is migrated; update native declarations, Max wiring, docs,
and this file together.

Each executable feature has exactly one root runtime component: either a native
`consolidator.<feature>.mxe64`, a root `<Feature>.js`/
`consolidator.<feature>.js` state component, or the root `<Feature>.maxpat`
for a pure Max feature. Every feature also owns exactly one root
`consolidator.<feature>.controller.js`. The controller owns the local UI boundary and
forwards commands either directly to its root state component or, when the
native public command endpoint is the common bus, as an envelope through
BusHub. Direct component status and diagnostics return to the controller when
the component exposes them. Pure Max presentation features route their local
UI feedback through the controller and do not need a message-bus connection.
Controllers must not own DSP, EQ state, persistence, or feature domain logic.
A feature without UI behavior still has a forwarding placeholder controller.
`consolidator.filter.js` is the stateless Filter endpoint that
replaces the removed native Filter external while
`consolidator.filter.controller.js`
remains responsible only for UI controls and layout. `EqStorage.js` is the
EqStorage state component; its controller owns local UI/list routing but does
not store banks or route the message bus.
`Max/Features/Shared/JS/DictionaryReader.js` is the single JavaScript boundary
for reading a Max dictionary. It resolves a dictionary reference once,
deserializes it to a plain object, and exposes root keys directly, for example
`config.filters[1]`. Feature controllers define their own UI objects and their
own direct `thispatcher` commands; do not introduce a shared control registry.

`Max/Features/Approximator/Approximator.maxpat` is the Approximator feature
wrapper. Its local command inlet accepts `fit`, `listen 0|1`, and `clear`.
`consolidator.approximator.controller.js` converts those commands into
`approximator.fit`, `analyzer.difference`, and `approximator.clear` envelopes.
`analyzer.difference` carries `payload.value` as the strict integer `0` or
`1`; do not rename that field to `enabled`.
The controller consumes the native `ready 0|1` selector directly. Fit forwards
one request whenever Listen is enabled; native Approximator is the sole owner
of readiness validation and reports a specific error instead of allowing the
controller to discard a click silently. Fit and Listen are inactive only after
native `fit_started` and remain so until `fit_finished` or `error`. The
momentary Fit button is gated on its value `1` so one click emits exactly one
`approximator.fit` envelope.
Native readiness is derived only from a compatible live difference curve and
the selected-bank baseline built from the current `DeviceState`,
at least one defined filter, and the absence of a running fit. Clearing Listen
removes only the difference curve; the current-EQ baseline remains valid.
Every `device.state.changed` generation invalidates the retained difference curve
so a fit cannot combine a new bank baseline with an older analysis frame.
The feature receives the high-rate difference curve through
`---approximator.difference.inlet`. Approximator derives the selected-bank EQ
baseline synchronously from each complete `DeviceState`; readiness must not
depend on a one-shot curve event or feature load order. The message bus remains
internal to the feature. Native status is exposed as
`status <state> [values]`; native output commands are published back to
BusHub.

`Max/Features/Analyzer/Analyzer.maxpat` is an isolated feature with four
signal inlets and no outlets. It owns SpectrumView and connects to the common
message bus internally. SpectrumView inputs are, in order: current spectrum,
sidechain spectrum, difference curve, individual filter curves, and the total
curve calculated by Analyzer from the full DeviceStateStore state, and canonical
`filter.state` envelopes. The total curve is only the fifth SpectrumView layer.
The sixth inlet receives canonical handle state. Spectrum edits are emitted to
EqStorage as absolute `filter.set` values.

Analyzer's current stereo input must be the pre-EQ current signal. From the
state it adds the response of banks `1..selectedBankId` to the displayed
current spectrum and subtracts the same response from reference-minus-current.
Banks after the selected bank are intentionally excluded. Approximator builds
only the selected-bank response from the same `DeviceState`, because the full
fit target is `selected bank + residual difference`. Using the prefix would
fold earlier banks into the selected bank and accumulate error.

`BusHub` is transport and startup coordination, not domain logic. Analyzer,
Approximator, EqStorage, and every Filter publish `system.status`. The static
startup barrier contains `analyzer`, `approximator`, and `eq.storage`; dynamic
Filter instances are observable participants but do not block startup because
their count is not a BusHub concern. When the static set is ready, BusHub
  broadcasts `system.start` exactly once. EqStorage publishes the selected
  canonical `filter.state` on start and whenever a configured Filter reports
  ready, so startup remains
order-independent and idempotent. No FIFO, retry, or
acknowledgement queue is introduced until a concrete delivery requirement
needs one.
The barrier coordinates feature roots, not internal implementation objects:
EqChain and SpectrumView do not publish separate startup states.
EqStorage must synchronously load all filter definitions from
`Max/Config/FilterConfig.json` and initialize at least bank 1 before publishing
state. Max does not guarantee that its
`loadbang` runs before messages emitted by another abstraction's `loadbang`.
After persistent state replaces the in-memory dictionary, EqStorage overwrites
the definition section from the current configuration and backfills every
configured Filter into every bank before publishing the restored state.
When it receives `system.start`, EqStorage must publish the complete current
state and every selected-bank `filter.state`. This gives every consumer a valid selected bank
and definition set after the startup barrier.

`Max/Config/FilterConfig.json` is the source of truth for filter contracts and
startup UI configuration. `consolidator.filter.controller.js` loads that file
directly through `Dict.import_json`, keeps the dictionary alive, selects its
  slot from the feature argument, and sends the argument-free local
  `configure` command after validation. EqStorage independently loads the same file and writes
  all definitions into DeviceStateStore before publishing state. Do not introduce a configuration patcher or Max routing for
file loading. Filter parameters are defined per slot under `filters`; layout
overrides are defined once per filter type under `layouts`.
`consolidator.filter.controller.js` owns all Max control behavior: it reads
`controls`/`layouts`, emits `script sendbox` commands for position, visibility,
  enabled state, colors, and values. It sends local `configure`, `update`, and
`reset` commands directly to `consolidator.filter.js`, then updates controls only from the
Filter status outlet. The endpoint converts local control activity to the
documented interfeature `filter.control` command; raw Max UI selectors never
enter BusHub.
`consolidator.filter.js` publishes `filter.control` and `filter.reset` through
BusHub to EqStorage. Its `system.status` identifies the configured filter so
EqStorage can return canonical state independently of load order. SpectrumView publishes
`filter.set`; Approximator publishes `filter.set_many`. EqStorage writes every
mutation into DeviceStateStore, immediately returns canonical `filter.state` to the
Filter endpoint and SpectrumView, then publishes a coalesced state generation
to EqChain, Analyzer, and Approximator.
`EqChain` has no bank-selection or approximator command outlet. Filter also
publishes direct status `status values <normalized...> <bypass>` for controller
state synchronization and lifecycle status `status ready`. `Analyzer` publishes
`filter_curve <filterId> <active> <r> <g> <b> <a> <frequencyHz> <gainDb>
<type> <q> <qMin> <qMax> <curve...>`. `SpectrumView` stores active filter curves, draws each curve
with its configured color, and computes the thick summed line itself. It emits
`filter.set` with absolute frequency/gain or Q values; StateStore validates and
clamps the values. Holding Alt while
dragging a marker keeps its frequency and gain fixed and edits Q directly as a
logarithmically mapped absolute value.

`consolidator.filter.js` has one command inlet. Local `update` emits
`filter.control` with one normalized value and local `reset` emits
`filter.reset`. Incoming `filter.state` is the only source used to update local
controls. No Filter endpoint keeps a pending or authoritative state copy.

`Max/Features/Analyzer/consolidator.analyzer.spectrumview.js` is the `jsui` entry point owned by
the Analyzer feature. It keeps the
standard `list` callback required by `jsui`. Its
implementation is split into `SpectrumViewConfig.js` for shared state and
visual constants, `SpectrumViewGeometry.js` for coordinate conversion,
`SpectrumViewCurves.js` for curve aggregation and drawing, and
`SpectrumViewInput.js` for Max messages and pointer interaction.

Max JavaScript is organized by feature under `Max/Features/`: `Analyzer/`
contains analysis and spectrum visualization, `Filter/` contains filter UI
adapters and views, and `EqStorage/` contains bank storage. Shared envelope contracts live
under `Max/Features/Shared/JS/Messages/`. `Painters/` contains
self-contained `jspainter` scripts;
these scripts must not depend on `include` files because Max can execute a
`jspainterfile` in a separate runtime context. Root-level JS files are not
used by Max patchers.

### EQ bank storage

`Max/Features/EqStorage/EqStorage.maxpat` is the reusable bank UI abstraction and audio
wrapper. It owns two audio inlets and two audio outlets and contains
`Consolidator.EqChain`; the parent device does not own another EqChain.
Its JavaScript implementation lives in `Max/Features/EqStorage/JS/`.
`DeviceStateStore` is the sole owner of the canonical dictionary, generation,
revision, and publication scheduling. `EqStorage` coordinates commands and UI;
`BankFilter` is a bank-filter value object.
Shared protocol infrastructure lives in `Max/Features/Shared/JS/Messages/`.
`MessageEnvelope` owns the envelope fields and Max dictionary conversion;
`MessageFactory` is the only JS factory boundary. Legacy per-command message
classes and `MessageCodec` are not part of the project.

Native message payloads are declared as typed contracts in
`Consolidator.Shared/Messaging/Messages/`. A native component registers its
accepted types in its `ComponentHost` template. Max dictionary conversion
happens only in `Consolidator.MaxAdapter`; components consume typed messages
and must not parse payload fields inside their envelope handlers.
Storage rows are ordinary EQ banks with one-based IDs. Bank 1 is the initial
bank created at startup; new rows are appended at 2, 3, 4 and so on. The
user-facing list and all storage and protocol messages use this same ID.
Filter publishes `filter.control` and `filter.reset` through BusHub to
EqStorage. SpectrumView and Approximator publish `filter.set` and
`filter.set_many`. EqStorage writes them into DeviceStateStore. DeviceStateStore
contains filter definitions directly alongside every bank, absolute filter values, bypass
state, selected bank, runtime generation, and persisted revision. It publishes
  only addressed `device.state.changed` notifications with `stateName` and
  `generation`; EqChain, Analyzer, and Approximator read the same dictionary.
  All bank rows are active EQ layers; the
selected row controls editing only and does not select the audible EQ. EqChain
stores every bank layer and processes them in ascending row order. There is no request
or capture phase.
The UI accepts `initialize`, `bang`, `add [name]`, `remove`, `select <row>`,
`rename <row> <name>`, and `delete <row>`. Every bank has an ordinary generated
or user-defined name and is embedded and restored with the Live Set.
`DeviceStateStore` owns the complete in-memory device model. The root `Consolidator.amxd` owns both
`pattr eqStorageState` and the parameter-enabled
`pattrstorage eqStorageBanks`; EqStorage only transports the state
dictionary through its private third inlet and outlet. The pattrstorage uses
`paraminitmode 1`, so `store 1` updates the M4L parameter initial value saved
with the Live Set. Persistence remains disabled during startup filter events.
After `live.thisdevice -> deferlow`, the root recalls slot 1 first and then
sends `persistence_ready`; only then may EqStorage publish state commits. Do
not use an embedded state `dict`, a nested pattr, or a loadbang recall. Keep
EqStorage non-embedded while root-level pattrstorage persists each device
instance.
Analyzer publishes the total response to SpectrumView. Approximator builds the
selected-bank response directly from `DeviceState`; both remain absolute-dB
curves calculated by the shared EQ implementation.
The source `.amxd` embeds only a clean EqStorage default with one generated
bank and selected row 1. Runtime bank state
belongs to the Live Set's per-instance pattrstorage value and must never be
saved back as the device's `parameter_initial` template.
A new bank is populated directly from definitions stored in DeviceStateStore. Bank
selection immediately publishes canonical `filter.state` for every defined
Filter. Bank creation, selection, updates, renames, and removal produce one
state generation rather than command-specific state events. Approximator reads
the selected bank before fit. Fit results carry that same `bankIndex` through
`filter.set_many`; EqStorage writes them to that explicit row and republishes
the complete state generation.
The user-facing list displays all banks in reverse order, while each row keeps
its one-based bank ID. Adding a bank appends a new ordinary bank with a
generated name and default filter values. EqChain processes storage rows in
ascending order (`1..N`), matching the storage and protocol order.
New bank names are generated as deterministic adjective-noun pairs and
existing custom names are preserved.

### Filter contracts

Filter definitions are dynamic. Supported filter types are `gain`, `tilt`,
`lowshelf`, `highshelf`, and `peak`.

UI controls are declared once in the JSON `controls` section. A filter type
uses its `layouts` entry for overrides such as `position`, `visible`, and
`enabled`; every slot of that type receives the same layout. Each filter slot
stores its own `color` in `filters.<slot>`.
consolidator.filter.js has no UI-control model. UI control IDs and their Max varnames
belong exclusively to `consolidator.filter.controller.js`.

Persisted and DSP parameter values are always absolute: gain in dB,
frequency/pivot in Hz, and Q as Q. Only direct UI control values between a Max
control, Filter controller, `consolidator.filter.js`, and EqStorage use
normalized `0..1` through `filter.control` and the normalized projection in
`filter.state`. EqStorage converts them from the contract ranges. NLopt may use
unit solver coordinates internally, but those coordinates never enter any
other envelope, device state, or persisted state. Supported parameter scales
are `linear` and `logarithmic`.

For `tilt`, the frequency parameter is named `pivot` everywhere. Do not
introduce a second name such as `freq` for that parameter.
UI control messages use semantic control IDs: `gain`, `frequency`, `q`, and
`bypass`. The UI must always emit `frequency`; EqStorage maps it to the contract
parameter `freq` or `pivot`. Do not emit `freq` as a UI control ID.

## Mandatory rules

1. Every command inlet and command outlet must explicitly document the full
   command list it accepts or produces. Keep the list in the Min inlet/outlet
   description and update it whenever a command is added, removed, renamed, or
   changes its payload. Every executable Max `js` or `jsui` entry script with
   ports must register `setinletassist` and `setoutletassist` callbacks that
   call `assist()` with the same contract.
2. Keep command routing explicit. Do not infer command meaning from argument
   count, value ranges, outlet position, or undocumented fallback behavior.
3. Keep shared EQ DSP math in `Consolidator.Shared`. Analyzer, EqChain, and
   Approximator must use the same filter formulas, frequency grid, and
   sample-rate assumptions. EqStorage performs normalized control-range
   conversion from the same JSON contract; `consolidator.filter.js` does not
   implement parameter math or DSP.
4. Preserve the dictionary contract across the dynamic filter flow. Do not
   serialize a dictionary to an ad-hoc string when a Max dictionary atom can be
   passed directly.
5. Normalization is private to Filter's local UI boundary. Only
   `filter.control` and the UI projection in `filter.state` carry normalized
   values. DeviceState, optimizer results, graph edits, and DSP specifications
   are absolute. Spectrum and EQ curves are absolute dB values.
   Filter colors are configuration data, not runtime `color` commands.
6. Preserve standardized inlet and outlet ordering. When migrating a non-DSP
   external, its first inlet/outlet become the envelope bus and its second
   outlet becomes the direct status outlet. For Min `sample_operator` objects,
   signal inlets remain first and the command bus follows them. Update C++
   declarations, Max patch wiring, descriptions, and documentation together.
7. Status and error messages must have one owner and one clear meaning. Do not
   emit duplicate lifecycle states from multiple paths.
8. Avoid accumulating state between fit runs unless it is explicitly part of
   the contract. A new capture or fit must define which buffers and active
   filter state are used.
9. Prefer separate classes and files with intent-revealing names. Keep Max
   patchers focused on routing and presentation; keep numerical behavior in
   C++.
10. Every executable Max feature has one root runtime component and one root
    `consolidator.<feature>.controller.js`. The root component is named
    either `consolidator.<feature>.mxe64` for native code,
    `<Feature>.js`/`consolidator.<feature>.js` for JavaScript state code, or
    `<Feature>.maxpat` for pure Max code. Its wrapper must route local UI
    through the controller, route direct component status back to the
    controller when available, and use BusHub only for interfeature envelopes.
    Every JavaScript or `jsui` file instantiated by a `.maxpat` must be in the
    owning feature root and named `consolidator.<feature>.<role>.js`; this also
    applies to controllers and views. Runtime filenames must be unique across
    features because Max caches dependencies by filename. Keep painters,
    includes, contracts, models, and other helper classes in `JS/` or the
    appropriate shared directory. Do not add bridge, adapter, or command entry
    scripts. New JS code belongs in the owning feature directory.
11. When reading code, treat a discovered violation of these rules as part of
   the task: fix it when the fix is local, behavior-preserving, and can be
   verified. Do not leave an obvious protocol or architecture mismatch
   unexplained.
12. Do not change Max patch wiring or behavior incidentally while editing C++.
   Make patch changes only when the requested behavior requires them.
13. Use ASCII for new source and documentation unless a file already requires
   another encoding.
14. Use PascalCase for class, struct, enum, and method names. Use camelCase
for variables, parameters, fields, and local functions. Do not introduce
snake_case identifiers outside external APIs that require them.
15. This project has no compatibility layer. Do not add migrations, legacy
format readers, fallback command formats, aliases, or dual protocol paths.
When a contract changes, replace the old contract directly and test the new
one from a clean state.
16. BusHub transports only interfeature envelopes. A feature controller must
    send UI actions directly to its local state component and update UI only
    from that component's direct status outlet. Never put UI control commands
    or types such as `filter.control.update` on the message bus.
17. Native Max components use `Consolidator.MaxAdapter/ComponentHost` for
    control input and `ComponentOutputs` for typed envelopes, lifecycle status,
    and diagnostics. Domain handlers are named `OnMessage` and
    `OnDeviceStateChanged`. Do not add component-local envelope deserialization,
    target routing, state-generation tracking, or message registries.

### Unified control envelope

The control protocol uses one envelope for every non-audio interfeature message:
`type`, `source`, `target`, and `payload`. Protocol types are stable names such
as `filter.state` and must not be tied to implementation class names.
`source` is always the feature that emitted the envelope. `target` is a feature
address or the literal `broadcast`; it is never an entity ID. Entity IDs,
including `filterId`, belong in `payload`. `MessageFactory` is the only factory
boundary in JavaScript; native components use the matching Shared
`MessageEnvelope` and `MessageFactory`. Max transports the envelope as
`message <temporary-dictionary-name>` through `Consolidator.MaxAdapter`; the
dictionary name is opaque transport
data and must never be routed or inspected by a component. Audio and high-rate
DSP buffers remain outside this protocol.
Components ignore broadcast envelopes unless their documented command contract
explicitly declares the broadcast type; a broadcast is never treated as a
generic request for every feature.

Stable flow contracts are:

- `system.status` carries `feature`, `state`, and an optional entity ID;
  `system.start` is the one startup broadcast.
- `filter.control` carries `filterId`, a semantic control ID, and one normalized
  `value`; it is the local UI boundary command.
- `filter.set` carries `filterId`, optional `bankIndex`, and absolute graph
  fields (`frequency` and `gain`, or `parameter: q` and `value`).
- `filter.set_many` carries `filterId`, `bankIndex`, and complete absolute
  `values`; it is the Approximator result command.
- `filter.reset` carries `filterId` and resets the selected bank row to the
  definition defaults.
- `filter.state` carries the complete canonical absolute and normalized state
  for one filter. EqStorage emits it immediately after a mutation and bank
  selection; Filter UI and SpectrumView consume it.
- `device.state.changed` carries only `stateName` and volatile `generation`.
  EqChain, Analyzer, and Approximator read all definitions, banks, selected-bank
  state, and absolute filter values from that one dictionary.

Raw control events never increment the persisted `revision`. StateStore returns
canonical `filter.state` synchronously, so controls and handles never maintain
an optimistic state that can snap back later. Heavy consumers use
latest-state-wins `device.state.changed` publication at an 8 ms control interval.
Persistence is committed after 250 ms of inactivity and increments `revision`
once.

Do not encode command-vs-event behavior as a boolean flag. Use the distinct
`filter.control`, `filter.set`, `filter.set_many`, `filter.reset`, and
`filter.state` types so every edge has one direction and one owner. When the
persisted EqStorage schema changes, bump
its schema version and create clean state; do not read or migrate the previous
format.

After `MessageFactory.fromMax`, JavaScript components use
`message.payload.<field>` directly. Envelope serialization and deserialization
happens only inside `MessageEnvelope`; configuration dictionaries use
`DictionaryReader`. Feature controllers must not read `payload::...` paths or
implement their own envelope parsing.

Do not add selector-specific message classes, `MessageCodec`, raw command
fallbacks, or compatibility handlers. UI-only Max commands stay direct within
their feature and must never be serialized into an envelope or sent to BusHub.

## Verification

Run `.vscode/build-all.cmd` from the repository root to configure and build
all native externals. The build uses Visual Studio x64, CMake, and vcpkg with
the static x64 triplet. Max may lock files in an owning
`Max/Features/<Feature>` directory; a successful native compile can still fail
only at the final copy step if Live or Max has the external loaded.

Before finishing a change:

- validate modified `.maxpat` files as JSON;
- run `git diff --check`;
- build the affected external, or all externals when a shared header or
  protocol changes;
- verify command names, inlet/outlet order, and payloads against this file.

## Keeping This File Current

Update this document when a key architectural decision changes: component
ownership, command protocol, dictionary schema, normalization rules, shared
constants, audio/curve flow, or build layout. Keep the description factual and
short; record stable rules here, not temporary debugging notes.
