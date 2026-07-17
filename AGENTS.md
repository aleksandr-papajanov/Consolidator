# Consolidator Project Rules

## Project

Consolidator is an Ableton Live / Max for Live audio tool for comparing a
current signal with a reference signal, fitting an EQ, and applying the
result through dynamically defined filter objects.

The repository has two layers:

- `Max/` contains Max for Live patchers, UI views, routing, configuration, and
  the built `.mxe64` externals used by Live.
- `Native/Consolidator/` contains the C++20 Min-DevKit project. Its Max
  externals are built from `Source/Projects/`.

## Architecture

### Native components

- `Consolidator.EqCore` is the shared domain layer. It owns filter types,
  parameter ranges and normalization, biquad/filter math, filter contracts,
  dictionary contract parsing, filter registry, and the runtime filter chain.
- `Consolidator.Analyzer` receives current and reference stereo signals and
  publishes spectrum curves and their difference.
- `Consolidator.Filter` represents one dynamically defined filter. It owns
  the filter contract and curve, accepts normalized parameter changes, and
  publishes commands for the chain.
- `Consolidator.EqChain` owns the active filter chain and processes stereo
  audio. It does not publish graphics data; filter visualization is published
  by each `Filter` instance.
- `Consolidator.Approximator` stores the available filter contracts, receives
  the difference curve and current EQ curve, and fits normalized filter
  parameters.

### Max data flow

The normal fitting path is:

`Analyzer -> difference curve -> Approximator -> Filter -> EqStorage -> EqChain`

The audio path is:

`audio -> EqStorage (EqChain) -> audio`

The visual path is:

`Analyzer / Filter -> SpectrumView or FilterCurveView`

`Max/Config/FilterConfig.json` is the source of truth for filter contracts and
startup UI configuration. `FilterConfig.maxpat` loads that JSON at
initialization, selects a filter dictionary, and sends it to the local command
inlet of `Consolidator.Filter`. Filter parameters are defined per slot under
`filters`; layout overrides are defined once per filter type under `layouts`.
The filter emits local `filter.control` envelopes. `FilterControlAdapter.maxpat`
converts them into Max `script sendbox` messages. `FilterCommandBridge.js`
wraps UI and configuration input before it reaches the native object.
Incoming parameter changes use `filter.control.update`; `Filter` publishes
`filter.define`, `filter.update`, and `filter.bypass`
envelopes directly to EqStorage. EqStorage owns the routes to its internal
EqChain and to Approximator. `EqChain` only consumes filter definitions and
audio commands; it has no bank or approximator command outlet. `Filter` also
publishes `filter_curve <filterId> <active> <r> <g> <b> <a> <frequencyHz>
<gainDb> <type> <q> <qMin> <qMax> <curve...>` and
draggable graph metadata as `handle <filterId> <frequencyHz> <gainDb> <type>
<active> <q> <qMin> <qMax>`. `SpectrumView` stores the active filter curves, draws each curve
with its configured color, and computes the thick summed line itself. It emits
`edit <filterId> <frequencyHz> <gainDb>` or `edit <filterId> q <normalizedValue>`;
`Filter` converts graph values to normalized parameters. Holding Alt while
dragging a marker keeps its frequency and gain fixed and edits Q directly as a
normalized value from 0 at the bottom to 1 at the top.

`Consolidator.Filter` has one command inlet. `filter.edit` is reserved for
SpectrumView and carries absolute graph coordinates (frequency/gain) or a
single normalized Q gesture. `filter.update` carries the complete normalized
parameter vector used for bank recall and state persistence; do not merge the
two message types or infer one from the other's payload.

`Max/JavaScript/Spectrum/SpectrumView.js` is only the Max JS entry point. Its
implementation is split into `SpectrumViewConfig.js` for shared state and
visual constants, `SpectrumViewGeometry.js` for coordinate conversion,
`SpectrumViewCurves.js` for curve aggregation and drawing, and
`SpectrumViewInput.js` for Max messages and pointer interaction.

Max JavaScript is organized by feature under `Max/JavaScript/`: `Spectrum/`
contains spectrum visualization, `Filter/` contains filter UI adapters and
views, `EqStorage/` contains bank storage, and `Messages/` contains shared
message contracts. `Painters/` contains self-contained `jspainter` scripts;
these scripts must not depend on `include` files because Max can execute a
`jspainterfile` in a separate runtime context. Root-level JS files are not
used by Max patchers.

### EQ bank storage

`Max/EqBankStorage.maxpat` is the reusable bank UI abstraction and audio
wrapper. It owns two audio inlets and two audio outlets and contains
`Consolidator.EqChain`; the parent device does not own another EqChain.
Its JavaScript implementation lives in `Max/JavaScript/EqStorage/`:
`EqStorage` coordinates bank state and `BankFilter` owns one filter's values
and bypass state.
Shared protocol infrastructure lives in `Max/JavaScript/Messages/`.
`MessageEnvelope` owns the envelope fields and Max dictionary conversion;
`MessageFactory` is the only JS factory boundary. Legacy per-command message
classes and `MessageCodec` are not part of the project.

Native message payloads are declared as value types in
`Consolidator.EqCore/TypedMessages.h`. Each type owns its stable protocol type
name and `from_envelope` deserializer. Components dispatch the registered
types through `protocol::dispatch` and implement small typed handler overloads;
do not add selector chains that parse payload fields inside `envelope_message`.
The list always contains `Current` at row 0; persistent rows start at row 1.
`Filter` publishes `filter.define`, `filter.update`, and `filter.bypass`
directly to EqStorage. EqStorage saves each update in the
selected row immediately and forwards definitions to Approximator. It also
publishes a complete `eq.storage.snapshot` to its internal EqChain after every
bank mutation. All bank rows, including `Current`, are active EQ layers; the
selected row controls editing only and does not select the audible EQ. EqChain
stores every bank layer and processes them in row order. There is no request
or capture phase.
The UI accepts `initialize`, `bang`, `add [name]`, `remove`, `select <row>`,
`rename <row> <name>`, and `delete <row>`. `Current` is a normal bank with the
fixed storage ID `current`, so it is embedded and restored with the Live Set.
`EqStorage` owns the complete in-memory bank model and publishes its full
snapshot after every mutation. The root `Consolidator.amxd` owns both
`pattr eqStorageState` and the parameter-enabled
`pattrstorage eqStorageBanks`; EqBankStorage only transports the state
dictionary through its private third inlet and outlet. The pattrstorage uses
`paraminitmode 1`, so `store 1` updates the M4L parameter initial value saved
with the Live Set. Persistence remains disabled during startup filter events.
After `live.thisdevice -> deferlow`, the root recalls slot 1 first and then
sends `persistence_ready`; only then may EqStorage publish state commits. Do
not use an embedded state `dict`, a nested pattr, or a loadbang recall. Keep
EqBankStorage non-embedded while root-level pattrstorage persists each device
instance.
A new bank sends
`filter.reset` to every defined Filter; the resulting update messages populate
the new bank with defaults. Restoring a bank sends its stored
`filter.update`/`filter.bypass` values. Every create, select, update, rename,
and removal publishes the envelope `eq.storage.bank.changed` with
the action, bank index, bank name, and relevant filter data.

### Filter contracts

Filter definitions are dynamic. Supported filter types are `gain`, `tilt`,
`lowshelf`, `highshelf`, and `peak`.

UI controls are declared once in the JSON `controls` section. A filter type
uses its `layouts` entry for overrides such as `position`, `visible`, and
`enabled`; every slot of that type receives the same layout. Each filter slot
stores its own `color` in `filters.<slot>`.
The native `FilterControl` value object emits semantic `control`
for `move`, `show`, `hide`, `enable`, and `disable`.

Parameter values sent as filter commands are normalized to `0..1`. Contract
ranges define how values are denormalized. Supported parameter scales are
`linear`, `logarithmic`, and `discrete`.

For `tilt`, the frequency parameter is named `pivot` everywhere. Do not
introduce a second name such as `freq` for that parameter.

## Mandatory rules

1. Every command inlet and command outlet must explicitly document the full
   command list it accepts or produces. Keep the list in the Min inlet/outlet
   description and update it whenever a command is added, removed, renamed, or
   changes its payload.
2. Keep command routing explicit. Do not infer command meaning from argument
   count, value ranges, outlet position, or undocumented fallback behavior.
3. Keep shared EQ math in `Consolidator.EqCore`. Analyzer, Filter, EqChain, and
   Approximator must use the same filter formulas, frequency grid, sample-rate
   assumptions, and parameter normalization rules.
4. Preserve the dictionary contract across the dynamic filter flow. Do not
   serialize a dictionary to an ad-hoc string when a Max dictionary atom can be
   passed directly.
5. Keep normalized control messages separate from absolute curve data. Filter
   parameters use normalized values; spectrum and EQ curves use absolute dB
   values. Filter colors are configuration data, not runtime `color` commands.
6. Preserve inlet and outlet ordering. If ordering changes, update the C++
   declarations, Max patch wiring, descriptions, and documentation together.
7. Status and error messages must have one owner and one clear meaning. Do not
   emit duplicate lifecycle states from multiple paths.
8. Avoid accumulating state between fit runs unless it is explicitly part of
   the contract. A new capture or fit must define which buffers and active
   filter state are used.
9. Prefer separate classes and files with intent-revealing names. Keep Max
   patchers focused on routing and presentation; keep numerical behavior in
   C++.
10. Max JavaScript must follow the same structure: keep the Max entry point
    thin, split domain responsibilities into separate files and classes, and
    use intent-revealing objects instead of large collections of global state
    and functions. New JS code belongs in the owning feature directory.
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

### Unified control envelope

The control protocol uses one envelope for every non-audio message:
`type`, optional `target`, optional `source`, and `payload`. Protocol types are
stable names such as `filter.update` and must not be tied to implementation
class names. `MessageFactory` is the only factory boundary in JavaScript;
native components use the matching `MessageEnvelope` and factory in
`EqCore`. Max transports the envelope as `message <temporary-dictionary-name>`;
the dictionary name is opaque transport data and must never be routed or
inspected by a component. Audio and high-rate DSP buffers remain outside this
protocol.

Do not add selector-specific message classes, `MessageCodec`, raw command
fallbacks, or compatibility handlers. UI-only Max commands may stay direct
only after the control envelope has been decoded at the UI adapter boundary.

## Verification

Run `.vscode/build-all.cmd` from the repository root to configure and build
all native externals. The build uses Visual Studio x64, CMake, and vcpkg with
the static x64 triplet. Max may lock files in `Max/Externals`; a successful
native compile can still fail only at the final copy step if Live or Max has
the external loaded.

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
