# Max V8 runtime diagnostics

This document defines the supported way to diagnose and rebuild the Max for
Live UI after the V8 migration. It also records known false leads so they are
not repeated.

## Runtime lifecycle

The supported host is Max 9 inside Live 12. The bridge contains two `v8`
entrypoints:

- `ConsolidatorUiHost.js` owns the UI application and protocol client;
- `LiveInstanceHost.js` owns Live identity and selection observers.

`live.thisdevice` sends its ready bang to both entrypoints. The UI host starts
idempotently from `loadbang` and `live_ready`. The Live host must not access
`LiveAPI` before that bang.

The expected activation sequence is:

```text
live.thisdevice bang
→ resolve this_device and its canonical parent track
→ publish track_name
→ publish instance_active 1 for the resolved `this_device`
→ observe selected_track for later focus changes
→ set_instance_active acknowledgement
→ observe_target
→ target_state_snapshot
→ resume control bindings
```

Controls can exist and paint their local defaults before the snapshot. Empty
dial values and an empty bank manager therefore mean that the drawing runtime
loaded but authoritative presentation has not yet been applied.

## Correct LiveAPI usage

The `LiveAPI` constructor receives callback first and path second. A read-only
accessor without a callback is created with an explicit `null` callback:

```javascript
const device = new LiveAPI(null, "this_device");
const track = new LiveAPI(null, "this_device canonical_parent");
```

Do not pass the path as the first argument. Do not read `canonical_parent`
through `get`; navigate to the canonical-parent path.

A `LiveAPI` callback is invoked both for observed property changes and for
object binding. The object-binding notification has an `id` selector and is
not a property value. The track-name and selected-track observers validate
their selectors:

```javascript
function trackNameChanged(values)
{
    if (!values || values.length < 2 ||
            String(values[0]) !== "name")
    {
        return;
    }
    // Apply the track name.
}
```

Activation starts with the device instance itself: after `this_device` and its
canonical parent track resolve successfully, the host publishes
`instance_active 1`. The selected-track observer then owns subsequent
transitions away from and back to the device. The host does not poll Live or infer activation
from structural `tracks`/`devices` changes.

## Diagnostic layers

Check the boundary in this order:

1. `Consolidator.Managed` registration proves only that the Native external
   loaded. It does not prove UI initialization.
2. Visible `v8ui` outlines prove that drawing objects loaded. They do not prove
   that a target snapshot was applied.
3. Confirm `instance_active 1` reaches `ConsolidatorUiHost`.
4. Confirm the protocol order `initialize`, `set_instance_active`,
   `observe_target`, `target_state_snapshot`.
5. Validate the snapshot size. The current snapshot has six header atoms and
   six atoms per entry.
6. If a `set_instance_active 0` occurs before the snapshot, inspect Live
   observer selectors before changing protocol or bindings.

Temporary diagnostics must be removed after the boundary is confirmed. Do not
retain `post` calls as fallback logging in product entrypoints.

The Max console warning `device project: device patcher has no name!` is not an
initialization failure. It can coexist with successful Native registration,
V8 loading and snapshot delivery.

## Host and Node test boundaries

Node tests prove deterministic CommonJS, ViewModel, presenter, binding and
protocol behavior. They do not provide the real `LiveAPI`, `patcher`,
`mgraphics`, `Task` or Max message dispatcher.

`LiveInstanceHostTests.js` must enforce the real `(callback, path)` constructor
shape, the own-device activation path and later selected-track/device callbacks.
A fake that accepts a path as the first constructor argument hides a host-only
failure.

The Max host harness must verify actual message dispatch, not only that named
objects exist. A result written before routing a message can be a false pass.
Errors such as `no function list` must first be checked for script path/load
failure. V8 supports rest parameters in Max-facing functions; do not replace
them based only on a harness whose script did not load.

Opening `ConsolidatorBridge.maxpat` directly outside its device project can
produce `Project:/...` resolution failures. Those errors do not reproduce the
packaged device environment and must not be used to diagnose the Live-hosted
runtime.

## Rebuilding Consolidator.amxd

Standalone Max opens the device with saving disabled and cannot rebuild the
M4L artifact correctly. Rebuild through Live 12:

1. drag `Consolidator.Max/Consolidator.amxd` onto a Live track;
2. open the device in the Max editor with the edit button;
3. save over the same `.amxd` from that editor;
4. close the editor, remove the device and add it again;
5. inspect the dependency cache for the current hosts and application modules.

The cache must contain `ConsolidatorUiApplication.js`, `LiveInstanceHost.js`
and `PanelBindingHostV8.js`. It must not contain removed legacy hosts such as
`LiveTrackIdentityHost.js` or `PanelBindingHost.js`.

Resaving the `.amxd` refreshes packaged dependencies but does not fix a
runtime lifecycle bug. Conversely, a source-only JavaScript fix is not active
in an already instantiated `v8` object; remove and reload the device before
host verification.

Do not automate Live UI input as part of routine diagnosis. Ask the operator to
load or select the device, then use the resulting Max log. This avoids changing
Live selection while selection state itself is under investigation.
