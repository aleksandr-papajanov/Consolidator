# Fixed VST3 channel strip

## Composition and scaling

Two fixed logical canvases: 1788 × 1104 with Track List expanded and 1212 × 1104
collapsed. A new editor starts expanded at 100%. The zoom button cycles
75/100/125%; free resizing is disabled. A uniform JUCE transform changes window
size and hit-test coordinates without reflow or changing child slots.

A 112-unit sidebar remains visible with Show/Hide Tracks, Undo, Redo, zoom and
Group/Local. The Track List panel is 576 units wide; its 544-unit table starts
at y=94 with 16-unit side margins. Consolidator and the horizontal
Group/Ungroup/Clear/Reset All row sit above it. Button bounds derive from the same
panel width. Rows/banks/S/M/R/B use 24-unit square geometry and shared borders;
five processor markers have 12-unit hit pitch and 8-unit indicators.

Collapse hides existing controls without changing target, models or subscriptions.
The processing area translates from x=688 to x=112 and retains its 1100-unit
width. Its title uses the observed registry label and shares the table's
InstanceButtonsControl. The S/M/R/B strip follows the title, reserving space
when the name is long. Local visibility/zoom are not Managed state or persistence.
The Inactive overlay covers the current full canvas; navigation is locked during
gestures and target transitions.

The strip has four sections across five column units: Gain Staging uses 440,
Saturator/Compressor/Polish 220 each. Headings are painted from presentation
metadata independently of snapshot/rebuild timing. Gain Staging B/R, Input/Output
subcolumns and Utility/Width are detailed below. The state contract belongs to
[Managed Gain Staging](../../../Consolidator.Managed/Docs/Aspects/GainStaging.md).
There is no processor panel selector, adaptive grid, parameter viewport, bank
dropdown or separate detector graph.

Equalizer internal geometry: [EqualizerLayout](EqualizerLayout.md).

## Shared bank and filter interaction

The vertical bank column remains at the left edge beside the graph when the
Track List is collapsed. Both bank views borrow TracksViewModel, BankCellRenderer
and BankCellInteraction: select/focus, Shift selection/drag with one bank per
instance, Ctrl bypass and Ctrl-double-click reset. Group/focus/selection/activity
and bypass feedback share one implementation. Each view owns only its pointer
visited set; there is no independent application selection store.

EQ and detector frequency/gain/Q editing is graphical-only. Handles retain drag,
Alt-drag Q, double-click reset and Ctrl-click bypass through Managed. Bypassed
handles and individual response curves remain visible in gray and editable.
There are no numbered filter buttons or duplicate filter dials. Graph gestures
have no Rough/Precise mode or corresponding snapping grid; Shift precision is
specific to dial editing. See [Controls](Controls.md) and
[AnalyzerRendering](AnalyzerRendering.md).


Доставка принятых значений и lifetime: [SnapshotAssembly](SnapshotAssembly.md).

## Fixed channel strip layout

The logical processing width remains 1100. Gain Staging occupies the first 440
units; Saturator, Compressor and Polish retain 220 units each. Beneath the Gain
Staging title, centered adjoining 28-unit JUCE B/R buttons control the whole state
subtree. B is an accepted-state toggle; R uses the existing reset command and
navigation lock. R/B do not bypass gesture/history or group-scope rules.

Below are two 220-unit subcolumns. Input contains Level, Input Level and Input
Target; Output contains Level, Output Level, Output Target and Limit, in that
order. A Utility heading and centered Width dial follow both columns. This is
fixed geometry, uniformly scaled by the editor's existing zoom. Scalar controls
retain numeric editing, Shift snapping, reset and group markers.

GainStagingBindings owns the existing InputLevelerViewModel. Its runtime overlay,
read-only input-level gesture behavior and slow/fast telemetry use the new paths
`gain_staging.input_level` and `gain_staging.input_level_fast`. Bindings outlive
parameter controls/models; rebuild destroys controls before borrowed models.
There is no parallel frontend value store or simulated output leveler feedback.

Track-table processor markers use five IDs: Gain Staging, Saturator, Compressor,
Equalizer and Polish. Gain Staging has one combined activity/bypass projection.
TargetStateClient stages five matching observe_target contexts, with Equalizer
last, and publishes the parameter union after every matching reply arrives.
