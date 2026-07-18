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

- `Consolidator.EqCore` is the shared domain layer. It owns filter types,
  parameter ranges and normalization, biquad/filter math, filter contracts,
  dictionary contract parsing, filter registry, and the runtime filter chain.
- `Consolidator.Analyzer` receives current and reference stereo signals and
  publishes spectrum curves and their difference. Curve delivery is scheduled
  on the Max main thread with a Min `queue<>` immediately after an FFT frame is
  completed; do not poll Analyzer with `qmetro` or an `analyzer.publish`
  message. Disabling `analyzer.difference` resets its difference smoothing;
  the Approximator feature also clears the retained fit curve and the
  Spectrum difference layer so stale data cannot remain ready or visible.
  Analyzer also owns individual filter visualization: it receives targeted
  `filter.define`, `filter.update`, and `filter.bypass` envelopes, then builds
  `filter_curve` with the shared filter contract and EQ math.
- `Max/Features/Filter/consolidator.filter.js` represents one dynamically defined
  filter. It owns the filter contract, normalized parameter state, bypass
  state, and publishes `filter.define`, `filter.update`, and `filter.bypass`
  envelopes for EqStorage and Analyzer. It does not process audio or publish
  visual curves or handles. `Filter.maxpat` sends its abstraction slot as
  `slot <id>` before the definition envelope is emitted.
- `Consolidator.EqChain` owns the active filter chain and processes stereo
  audio. It does not calculate or publish visual curves.
- `Consolidator.Approximator` stores the available filter contracts, receives
  the difference curve and current EQ curve, and fits normalized filter
  parameters.

### Max data flow

The normal fitting path is:

`Analyzer -> difference curve -> Approximator -> Filter -> EqStorage -> EqChain`

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
`consolidator.filter.js` is the Filter state component that
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
The controller consumes the native `ready 0|1` selector directly. Fit is
active only while Listen is enabled and native ready is `1`; both Fit and
Listen are inactive during a fit. The momentary Fit button is gated on its
value `1` so one click emits exactly one `approximator.fit` envelope.
Native readiness is derived only from compatible difference/current-EQ curves,
at least one defined filter, and the absence of a running fit. Clearing Listen
removes only the difference curve; the current-EQ baseline remains valid.
The feature receives the high-rate difference and current-EQ curves through
`---approximator.difference.inlet` and `---spectrum.eqcurve.outlet`, while the
message bus remains internal to the feature. Native status is exposed as
`status <state> [values]`; native output commands are published back to
BusHub.

`Max/Features/Analyzer/Analyzer.maxpat` is an isolated feature with four
signal inlets and no outlets. It owns SpectrumView and connects to the common
message bus internally. SpectrumView inputs are, in order: current spectrum,
sidechain spectrum, difference curve, individual filter curves, and the total
curve calculated by Analyzer from the full EqStorage snapshot. Analyzer sends
that total curve to `---spectrum.eqcurve.outlet` for SpectrumView and
Approximator. Spectrum edits are emitted to the bus as `filter.edit`.
Do not connect this feature to the root device until it is verified alone.

`BusHub` is transport and startup coordination, not domain logic. During one
boot cycle each required feature publishes `system.status` with a ready state.
When the static required-feature set is ready, BusHub broadcasts
`system.start`. Feature startup must be idempotent. No FIFO, retry, or
acknowledgement queue is introduced until a concrete delivery requirement
needs one.

`Max/Config/FilterConfig.json` is the source of truth for filter contracts and
startup UI configuration. `Max/Features/Filter/FilterConfig.maxpat` loads that JSON at
initialization, selects a filter dictionary, and sends it to the local command
inlet of `consolidator.filter.js`. Filter parameters are defined per slot under
`filters`; layout overrides are defined once per filter type under `layouts`.
`consolidator.filter.controller.js` owns all Max control behavior: it reads
`controls`/`layouts`, emits `script sendbox` commands for position, visibility,
enabled state, colors, and values. It sends local `define`, `update`, and
`reset` commands directly to `consolidator.filter.js`, then updates controls only from the
Filter status outlet. UI-control changes never enter BusHub.
`FilterConfig.maxpat` has a strict local contract: its only outlet emits the
bare name of an existing Max dictionary (for example `u123456`) with outlet
type `dictionary`. It must not emit a JSON string, a `filters::N` path, or a
textual `dictionary` prefix. `consolidator.filter.controller.js` accepts
that dictionary name and opens it through `new DictionaryReader(name)`.
`consolidator.filter.js` publishes `filter.define`, `filter.update`, and `filter.bypass`
envelopes through `BusHub` to both EqStorage and Analyzer. EqStorage owns the routes to its internal
EqChain and to Approximator. `EqChain` only consumes filter definitions and
audio commands; it has no bank or approximator command outlet. `Filter` also
publishes direct status `status values <normalized...> <bypass>` for controller
state synchronization and lifecycle status `status ready`. `Analyzer` publishes
`filter_curve <filterId> <active> <r> <g> <b> <a> <frequencyHz> <gainDb>
<type> <q> <qMin> <qMax> <curve...>`. `SpectrumView` stores active filter curves, draws each curve
with its configured color, and computes the thick summed line itself. It emits
`edit <filterId> <frequencyHz> <gainDb>` or `edit <filterId> q <normalizedValue>`;
`Filter` converts graph values to normalized parameters. Holding Alt while
dragging a marker keeps its frequency and gain fixed and edits Q directly as a
normalized value from 0 at the bottom to 1 at the top.

`consolidator.filter.js` has one command inlet. `filter.edit` is reserved for
SpectrumView and carries absolute graph coordinates (frequency/gain) or a
single normalized Q gesture. `filter.update` carries the complete normalized
parameter vector used for bank recall and state persistence; do not merge the
two message types or infer one from the other's payload.

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
Its JavaScript implementation lives in `Max/Features/EqStorage/JS/`:
`EqStorage` coordinates bank state and `BankFilter` owns one filter's values
and bypass state.
Shared protocol infrastructure lives in `Max/Features/Shared/JS/Messages/`.
`MessageEnvelope` owns the envelope fields and Max dictionary conversion;
`MessageFactory` is the only JS factory boundary. Legacy per-command message
classes and `MessageCodec` are not part of the project.

Native message payloads are declared as value types in
`Consolidator.EqCore/TypedMessages.h`. Each type owns its stable protocol type
name and `from_envelope` deserializer. Components dispatch the registered
types through `protocol::dispatch` and implement small typed handler overloads;
do not add selector chains that parse payload fields inside `envelope_message`.
Storage rows are ordinary EQ banks with one-based IDs. Bank 1 is the initial
bank created at startup; new rows are appended at 2, 3, 4 and so on. The
user-facing list and all storage and protocol messages use this same ID.
`Filter` publishes `filter.define`, `filter.update`, and `filter.bypass`
through `BusHub` to EqStorage and Analyzer. EqStorage saves each update in the
selected row immediately and forwards definitions to Approximator. It also
publishes a complete `eq.storage.snapshot` to its internal EqChain and Analyzer
after every bank mutation. All bank rows are active EQ layers; the
selected row controls editing only and does not select the audible EQ. EqChain
stores every bank layer and processes them in reverse row order. There is no request
or capture phase.
The UI accepts `initialize`, `bang`, `add [name]`, `remove`, `select <row>`,
`rename <row> <name>`, and `delete <row>`. Every bank has an ordinary generated
or user-defined name and is embedded and restored with the Live Set.
`EqStorage` owns the complete in-memory bank model and publishes its full
snapshot after every mutation. The root `Consolidator.amxd` owns both
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
Analyzer publishes the summed response of all audible bank layers to
`---spectrum.eqcurve.outlet`; Spectrum and Approximator are consumers of that
shared absolute-dB curve.
The source `.amxd` embeds only a clean EqStorage default with one generated
bank, selected row 1, and no recovered `DeviceInit` flags. Runtime bank state
belongs to the Live Set's per-instance pattrstorage value and must never be
saved back as the device's `parameter_initial` template.
A new bank sends
`filter.reset` to every defined Filter; the resulting update messages populate
the new bank with defaults. Restoring a bank sends its stored
`filter.update`/`filter.bypass` values. Every create, select, update, rename,
and removal publishes the envelope `eq.storage.bank.changed` with
the action, bank index, bank name, and relevant filter data.
Approximator consumes the selected `bankIndex` before fit. Fit results carry
that same `bankIndex` through `filter.update`; EqStorage writes the result to
that explicit row and republishes the complete EqChain snapshot.
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

Parameter values sent as filter commands are normalized to `0..1`. Contract
ranges define how values are denormalized. Supported parameter scales are
`linear`, `logarithmic`, and `discrete`.

For `tilt`, the frequency parameter is named `pivot` everywhere. Do not
introduce a second name such as `freq` for that parameter.
UI control messages use semantic control IDs: `gain`, `frequency`, `q`, and
`bypass`. The UI must always emit `frequency`; Native Filter maps it to the
contract parameter `freq` or `pivot`. Do not emit `freq` as a UI control ID.

## Mandatory rules

1. Every command inlet and command outlet must explicitly document the full
   command list it accepts or produces. Keep the list in the Min inlet/outlet
   description and update it whenever a command is added, removed, renamed, or
   changes its payload. Every executable Max `js` or `jsui` entry script with
   ports must register `setinletassist` and `setoutletassist` callbacks that
   call `assist()` with the same contract.
2. Keep command routing explicit. Do not infer command meaning from argument
   count, value ranges, outlet position, or undocumented fallback behavior.
3. Keep shared EQ DSP math in `Consolidator.EqCore`. Analyzer, EqChain, and
   Approximator must use the same filter formulas, frequency grid, and
   sample-rate assumptions. `consolidator.filter.js` performs only normalized control-range
   conversion from the same JSON contract; it does not implement DSP math.
4. Preserve the dictionary contract across the dynamic filter flow. Do not
   serialize a dictionary to an ad-hoc string when a Max dictionary atom can be
   passed directly.
5. Keep normalized control messages separate from absolute curve data. Filter
   parameters use normalized values; spectrum and EQ curves use absolute dB
   values. Filter colors are configuration data, not runtime `color` commands.
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

### Unified control envelope

The control protocol uses one envelope for every non-audio interfeature message:
`type`, `source`, `target`, and `payload`. Protocol types are stable names such
as `filter.update` and must not be tied to implementation class names.
`source` is always the feature that emitted the envelope. `target` is a feature
address or the literal `broadcast`; it is never an entity ID. Entity IDs,
including `filterId`, belong in `payload`. `MessageFactory` is the only factory
boundary in JavaScript; native components use the matching `MessageEnvelope`
and factory in `EqCore`. Max transports the envelope as
`message <temporary-dictionary-name>`; the dictionary name is opaque transport
data and must never be routed or inspected by a component. Audio and high-rate
DSP buffers remain outside this protocol.
Components ignore broadcast envelopes unless their documented command contract
explicitly declares the broadcast type; a broadcast is never treated as a
generic request for every feature.

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
