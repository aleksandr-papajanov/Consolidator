# Max JavaScript V8 migration

## Target platform

The supported platform for this migration is Max 9 / Live 12. Live 11 and
older versions are outside the supported platform.

The runtime roles are:

- `v8` for protocol/client logic, ViewModels, presenters, bindings, routing
  and host orchestration;
- `v8ui` for `ButtonControl`, `DialControl`, `AnalyzerControl` and other
  drawing controls.

Cycling '74 documents `v8` as a generic Max JavaScript object and `v8ui` as
the object with drawing and interaction context. The older `js` and `jsui`
objects use the legacy JavaScript engine.

## Inventory

### JavaScript files

There are 63 `.js` files under `Consolidator.Max/js`. Max-facing controls are
direct `v8ui` entrypoints; reusable application layers are CommonJS modules.

| Category | Files | Target | Status |
| --- | ---: | --- | --- |
| Clients | 8 | `v8` library/module | CommonJS classes completed; consumed by the V8 host |
| ViewModels | 11 | `v8` library/module | all feature ViewModels migrated to CommonJS classes |
| Presenters | 15 | `v8` library/module | all presenters and presentation DTOs migrated to CommonJS |
| Bindings | 8 | `v8` library/module | CommonJS classes completed |
| Controllers | 8 | `v8` library/module | CommonJS classes completed |
| Hosts and application | 4 | `v8` object/module | three Max entrypoints plus `ConsolidatorUiApplication` |
| Drawing controls and support | 9 | `v8ui` object/support | six direct controls plus analyzer support modules |

The counts include `PanelBindingHostV8.js` in Hosts; the removed legacy host is
not part of the current file count.
The controls-scope files include the six drawing controls and
`AnalyzerLayout.js`, `AnalyzerRenderer.js` and `AnalyzerViewState.js`.

### Max objects in patchers

| Current object | Count | Files / role | Target |
| --- | ---: | --- | --- |
| `js` | 0 | no remaining legacy Max JavaScript objects | `v8` |
| `jsui` | 0 | no remaining drawing controls | `v8ui` |
| `v8ui` | 23 | `AnalyzerControl.js` (3), `BankManagerControl.js` (1), `ButtonControl.js` (6), `DialControl.js` (12), `HistoryPanel.js` (1) | retain as V8UI runtime |
| `v8` | 9 | `PanelBindingHostV8.js` in the panels and both hosts in the bridge | retain as V8 runtime |

The patchers containing these objects are the seven panel patchers under
`Consolidator.Max/Panels` plus `ConsolidatorBridge.maxpat` for the host objects.
`Main.maxpat` embeds the bridge through a `bpatcher`.

`v8ui` has no positional filename argument. Each drawing box stores its script
only in the `filename` attribute; a `text` value such as `v8ui file.js` is
invalid and prevents the UI object from being created.

### Max APIs and lifecycle hooks

| API / hook | Current usage | Target handling |
| --- | --- | --- |
| `include(...)` | 0 directives in product JS | modules use explicit CommonJS `require` |
| global variables | Max entrypoint state and message functions only | classes and helpers remain lexical or module-scoped |
| `Task` | two timers in `AnalyzerControl.js` | retain in `v8ui`; cancel during control teardown |
| `mgraphics` | six drawing controls | retain only in direct `v8ui` entrypoints; analyzer renderer receives a graphics context |
| `arrayfromargs` | 0 uses | V8 rest parameters normalize callback arguments |
| `outlet` | hosts and all drawing controls | retain at Max object boundaries only |
 | numeric list envelope | `ConsolidatorUiHost.js`, `PanelBindingHostV8.js` | bridge prefixes dynamic messages with sentinel `0`; V8 calls `list()` only for lists beginning with a number |
| `patcher.getnamed` | `PanelBindingHostV8.js` | retain with per-instance cache |

No product JS uses `patcher` lookup outside the panel router. No product JS
uses `setTimeout` or browser APIs.

### Tests

The Node test suite has one entrypoint and five client suites. Its production
module coverage is organized as follows:

- `UiBindingTests.js` — `PanelBindingHostV8.js` and direct V8UI entrypoint controls;
- `LiveInstanceHostTests.js` — CommonJS `LiveInstanceHost.js`;
- `RegistryAndBankManagerTests.js` — ViewModel and control loading;
- `RuntimePathTests.js` — include path inventory and runtime path checks.

The test environment supplies shims for `mgraphics`, `outlet` and `Task`.
CommonJS application modules use Node `require`; Max entrypoints are evaluated
as object scripts with a file-relative `require`. The V8 pilot has a dedicated
VM context in `UiBindingTests.js` and is also covered by the Max 9 host harness.

## Classification

- `ConsolidatorUiHost.js`, `LiveInstanceHost.js` and `PanelBindingHostV8.js`
  are Max objects targeting `v8`.
- Clients, ViewModels, presenters, bindings and controllers are CommonJS
  modules consumed by `v8` host code. `ConsolidatorUiHost.js` is a regular
  `v8` Max entrypoint. Drawing controls are direct `v8ui` entrypoints so
  `mgraphics`, `Task`, `outlet` and lifecycle callbacks stay at the Max boundary.
- The 23 controls instantiated by patchers are drawing objects and target
  `v8ui`.
- `PanelBindingHost.js` was an unreferenced legacy implementation and has been
  removed; all panel callers already use `PanelBindingHostV8.js`.
- `SliderControl.js` has no patcher reference or product include and requires
  an explicit usage decision before removal.

## Pilot acceptance criteria

Verify `PanelBindingHostV8`, the migrated `v8ui` controls and
`LiveInstanceHost` in Max 9 for:

1. `patcher.getnamed` lookup and one lookup per control id;
2. cache lifetime across repeated `list` and `anything` messages;
3. numeric list envelopes with sentinel `0`, control id and payload;
4. bridge normalization of Native callbacks, UI presentations and UI intents;
5. nested panel patchers and correct local patcher ownership;
6. teardown without callbacks using stale cached control objects.

The runtime conversion is complete when every patcher object and every test
fixture uses the same `v8`/`v8ui` contract. The device lifecycle, Live selection
observers, protocol snapshot and panel routing paths have been verified in
Max 9 hosted by Live 12. Runtime diagnosis and `.amxd` rebuild rules are in
[`MaxV8RuntimeDiagnostics.md`](MaxV8RuntimeDiagnostics.md).

`ConsolidatorUiHost` starts idempotently from both the Max `loadbang` callback
and the bridge's `live_ready` lifecycle message. The latter is required for a
host instantiated dynamically through the Max for Live device/bpatcher path,
where Max does not guarantee a JavaScript `loadbang` callback.
