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
Selecting Input, Saturator, Compressor, EQ or Output changes the local panel
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
