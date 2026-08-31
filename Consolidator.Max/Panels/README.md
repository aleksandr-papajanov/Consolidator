# Consolidator panel bridge

These panels are the first message-based bridge composition for
`ConsolidatorBridge.maxpat`.

## Path contract

The root patcher references panels from the Max Project. JavaScript files use
the project-root alias:

```text
Project:/Panels/EqualizerPanel.maxpat
Project:/Panels/BankManagerPanel.maxpat
```

Panel-local JS objects reference files through the Max Project:

```text
Project:/js/Controls/Analyzer/AnalyzerControl.js
Project:/js/PanelBindingHostV8.js
```

These paths are resolved from the Max Project root. The panels must be opened
through the project, not as standalone patchers, so that the `Project:/` alias
is available when Max reloads or saves them.

The parent patch sends presentation messages in this form:

```text
<control-id> <selector> <arguments...>
```

Each panel receives the complete stream and uses its local
`PanelBindingHostV8.js` to resolve only controls inside that panel. Control
outlets are explicitly prefixed with their stable control ID before returning
to the parent Host UI-intent inlet.

Dial and button availability follows the target-state status reactively. They
remain disabled while the target snapshot is unavailable and become enabled
when that same state source reports ready; disabled dials use the inactive
color.

Bank grouping uses the edit mode in the bank manager. A regular bank click
starts a new grouping selection, while Shift-click extends that selection
across banks and instances. Selected banks remain visibly active until a group
is applied or edit mode is closed.

The bank manager includes a 32-slot history timeline below its action buttons.
Slots keep the standard cell size and may be clipped by the available width.
The current cursor is kept in the center when possible; clicking a visible slot
navigates the shared history cursor through the existing `jump_history` command
and shifts the visible timeline window.

The Bank Manager also provides a vertical panel selector on its right side.
Selecting Input, Saturator, Compressor, EQ, Polish or Output changes the local panel
presentation. Target snapshots continue to follow the existing bank focus
transition.

The first bank-manager snapshot uses a complete presentation. Later registry
additions, removals, label changes and bank-group changes use row, bank and
group patch messages followed by one redraw. A delta therefore does not replay
every existing row through every loaded Max UI.
Focus changes patch only the previously selected and newly selected bank cells;
they do not rebuild or resend the registry table.

Panel routing currently runs in Max 9's `v8` object through
`PanelBindingHostV8.js`.

`AnalyzerPresenter` owns analyzer handles, calculated filter curves and streamed
spectrum data. Handle gestures use the ordinary state/history command path.
Dragging updates the local preview immediately and sends the latest position at
a bounded live rate. Curves are calculated locally from the focused parameter
presentation, the raw all-bank equalizer projection and the observed source's
prepared sample rate; spectrum and curve redraws are independently coalesced.

All Max UI RGBA colors are defined in `js/Theme/UiColors.js`. The config is
organized by semantic role: base colors, reusable control colors, bank-group
colors, device colors and analyzer colors. Controls and presenters consume
the shared config; new UI colors should be added there instead of being
embedded in a control or renderer.

State controls use the current global edit scope. Local scope writes the
selected control locally, while group scope writes it to the focused bank's
group. This applies to dials, filter handles, filter Q wheel edits, resets and
feature buttons.

The current edit scope is now a single UI-wide state owned by the client. The
Bank Manager Action Panel exposes a Scope toggle.
The toggle is disabled unless the focused bank belongs to a group; when group
scope is active, it is filled with the focused bank's group color. All regular
state writes and processor actions read this shared scope directly.

Selecting a grouped bank highlights all members of its group only while group
scope is active. In local scope, other group members are not highlighted;
explicitly selected banks remain highlighted for grouping.

Dial tracks use their normal full range for display and local editing. During
group scope, the track switches to the ring's effective minimum-to-maximum
range so group limits are visible while editing.

Controls that write through group scope show the same group color from
`UiEditScope`: feature buttons and Bank Manager S/M/R/B controls receive a
small marker, dial tracks use the group color, and analyzer filter handles
use the group color.

The Bank Manager Clear action sends the `clear_topology` command. Managed
clears all non-default topology groups (`groupId > 0`) atomically across the
registry. The default group `0` is preserved. Group, Ungroup and Clear each
create one history point for the complete topology operation.
