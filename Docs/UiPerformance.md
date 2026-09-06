# Max UI performance

Target snapshot application is a synchronous presentation batch. State values
may notify their subscribers while the batch is open, but presenters defer
rebuilds and each affected presenter rebuilds once when the batch closes.
Bindings remain suspended until the target transition completes, so the latest
presentation is sent once for each active control.

Inactive UI instances do not request an initial target snapshot. The first
activation requests the current target after Managed acknowledges activation.

`PanelBindingHostV8` caches named patcher controls because panel object identity is
stable for the lifetime of the panel. Button presentation updates are framed by
`presentation_begin` and `presentation_end`, allowing the control to redraw once
for the complete update. Analyzer redraw and drag scheduling reuse one task per
purpose rather than allocating a task for each update.

Analyzer visual redraw is coalesced at 16 ms for a responsive local marker and
curve preview. Authoritative drag writes remain coalesced at 33 ms, and FFT
production remains on its independent 33 ms cadence. Increasing visual frame
rate therefore does not increase Managed command or analysis frequency.

`PanelBindingHostV8` is the first modern-engine pilot. It uses an ES6 class,
`Map`, rest/spread arguments and destructuring, and is instantiated by `v8`
objects in all panels.

Panel selection remains local to the Max presentation. Bank focus changes still
drive the existing target transition and snapshot flow.

## Edit precision

The Bank Manager action row exposes the shared edit precision action. It toggles between
`PRECISE` and `ROUGH` for every dial and analyzer marker. Rough mode quantizes
dB values to 3 dB, other scalar values to 0.1, analyzer frequencies to ten
logarithmic steps per decade anchored at the analyzer minimum, and Q to five
logarithmically spaced values across its effective range. The mode is UI-only
and does not enter Managed state or DSP. A precision change publishes once to
each active control, and delayed gesture writes retain the snapped value when
their history transaction becomes ready.

## BankManager scrolling

`BankManagerControl` keeps track scrolling local to the Max UI control. When
the rows exceed the control height, the content can be dragged directly inside
the control; the mouse wheel moves by one row per wheel step. Scrolling is
clamped to the available content range and does not create Managed protocol
traffic.

Each row is laid out as three columns: the track name uses the remaining
available width, the processor marker column uses the width of its processor markers,
and the bank column uses the maximum bank count plus the instance controls.
Bank cells do not display their bank ID; only banks assigned to a group
display that group's alphabetic ID (`A` for group `0`). The bank grid is drawn
separately from the cells; selection fills the cell space without adding a
cell border, group labels are centered using the active font metrics, and
selecting a grouped bank highlights every bank in that group. Banks with an
active EQ effect show a small contrasting marker, including when the bank is
selected. This status is updated by a registry delta only when the
neutral/active state changes.

BankManager actions are rendered in a fourth column after the track name,
processor markers, and banks. The five action buttons are stacked vertically,
with `Redo` and `Undo` below them as a separate group with a small gap. The
history group contains only these two one-step controls.
A normal click starts a selection with one bank. Shift-click adds or removes
one bank, while Shift-drag across the bank grid toggles each crossed bank once;
the existing one-bank-per-track selection constraint still applies. `Group`
writes the next available
group ID to the selected banks, `Ungroup` clears the focused bank's group, and
`Clear` clears local groups with confirmation. The former link-group panel is
not part of the presentation.

History state is delivered through the same complete presentation stream as the
bank table; it does not create registry traffic. `Redo` and `Undo` issue the
corresponding one-step history request when enabled.

Group `0` is the automatic system group and cannot be changed or ungrouped.
User groups accept at most one bank from each track, and only ungrouped banks
can be selected for a new group; the same constraints are enforced again by
Managed when state writes are received.
After a successful group creation, the Bank Manager focuses the first bank of
the new group and activates its group context automatically.

Each track row exposes the instance-level `S` and `M` controls. Device and bank
solo controls are not part of the UI or protocol.

Each track name has a six-pixel lime output-level dot. Its alpha is derived
nonlinearly from the track's final 300 ms RMS telemetry over `-60..0 dBFS`.
Managed batches all instance values into one latest-only analysis frame, and the
Bank Manager applies dedicated `level_patch` updates without refetching or
rebuilding registry rows.

## BankManager control structure

The `BankManagerControl.js` V8UI entrypoint owns only Max callbacks. Feature
composition lives in `BankManagerControlCore`; its collaborators have one UI
responsibility each:

- `BankManagerLayout` calculates columns, rows, hit regions and scroll bounds;
- `BankManagerRenderer` draws the current presentation without producing intents;
- `BankManagerInteraction` translates pointer and wheel input into intents;
- `BankManagerPresentationState` assembles full presentations and applies patches;
- `BankManagerFeedback` owns short-lived action flashes and optimistic bypass values.

The presentation stream remains authoritative. Optimistic bypass values are removed
when the corresponding presentation confirms them. Full and patched bank records
both carry bypass state. All flash tasks are owned by the V8UI control and cancelled
from `notifydeleted`.

## JavaScript module boundaries

Max-facing drawing files are thin selector adapters. Button, Dial, Analyzer,
MultiValueToggle and BankManager keep reusable state and rendering outside their
entrypoints. Dial label timers and Analyzer scheduling are cancelled from
`notifydeleted`; delayed callbacks do not redraw a destroyed control.

The application host delegates component construction, control-binding
installation, varname mapping and track-name parsing to separate modules. Client
wire decoding is isolated from request/lifecycle orchestration: registry and state
snapshots have dedicated assemblers/codecs, while registry deltas have a dedicated
applier. Protocol subscriptions are released by each client during destruction.

Analyzer scale conversion, biquad curve calculation, curve publication and
parameter editing are separate responsibilities. The removed all-bank curve path
had no producer and is not part of the presentation contract. Dial ring generation
and value editing are likewise independent of its presenter, and gesture
transaction state is owned by the binding session rather than the message encoder.
Ring visibility is part of the dial presentation, so transient DSP indicators
can be hidden without changing ring topology or rebuilding a Max control.
Relative dial rings use the normalized midpoint as zero and draw only the span
from that center to the current value; they do not invent a minimum visible span.
