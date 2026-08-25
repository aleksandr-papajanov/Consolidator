# Consolidator panel bridge

These panels are the first message-based bridge composition for
`ConsolidatorBridge.maxpat`.

## Path contract

The root patcher references panels with the `Project:/` prefix:

```text
Project:/Panels/EqualizerPanel.maxpat
Project:/Panels/BankManagerPanel.maxpat
```

Panel-local JS objects must reference files relative to the `Panels/` folder
using the `Project:/` prefix:

```text
Project:/js/Controls/Analyzer/AnalyzerControl.js
Project:/js/PanelBindingHost.js
```

These paths are resolved relative to the patcher containing the object. They
remain reliable even when a panel is opened directly or saved from Max.

The parent patch sends presentation messages in this form:

```text
<control-id> <selector> <arguments...>
```

Each panel receives the complete stream and uses its local
`PanelBindingHost.js` to resolve only controls inside that panel. Control
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

The first bank-manager snapshot uses a complete presentation. Later registry
additions, removals, label changes and bank-group changes use row, bank and
group patch messages followed by one redraw. A delta therefore does not replay
every existing row through every loaded Max UI.
Focus changes patch only the previously selected and newly selected bank cells;
they do not rebuild or resend the registry table.

`AnalyzerPresenter` owns analyzer handles, calculated filter curves and streamed
spectrum data. Handle gestures use the ordinary state/history command path.
Dragging updates the local preview immediately and sends the latest position at
a bounded live rate; spectrum and curve redraws are independently coalesced.
