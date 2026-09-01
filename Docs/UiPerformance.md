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
processor markers, and banks. The four action buttons are stacked vertically,
with `Redo` and `Undo` below them as a separate group with a small gap. The
history group contains only these two one-step controls.
A normal click
starts a selection with one bank and Shift-click adds or removes banks from
it; `Group` writes the next available
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

Each track row exposes the instance-level `S` and `M` controls. Device and bank
solo controls are not part of the UI or protocol.
