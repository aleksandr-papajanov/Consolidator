# Fixed VST3 channel strip

## Composition and scaling

Two fixed logical canvases: 1788 × 904 with Track List expanded and 1212 × 904
collapsed. A new editor starts expanded at 100%. The zoom button cycles
75/100/125%; free resizing is disabled. A uniform JUCE transform changes window
size and hit-test coordinates without reflow or changing child slots.

A 112-unit sidebar remains visible with Show/Hide Tracks, Undo, Redo, zoom and
Group/Local. The Track List panel is 576 units wide; its 544-unit table starts
at y=94 with 16-unit side margins. Consolidator and the horizontal
Group/Ungroup/Clear/Reset All row sit above it. Tracks BankManagerControl owns
this entire panel, its action availability and geometry. Button bounds derive
from its panel width. Rows/banks/S/M/R/B use 24-unit square geometry and shared borders;
six passive processor markers have 12-unit pitch and 8-unit indicators.

Collapse hides existing controls without changing target, models or subscriptions.
The processing area translates from x=688 to x=112 and uses a 1100-unit
width. Its title uses the observed registry label and shares the table's
InstanceButtonsControl. The S/M/R/B strip follows the title, reserving space
when the name is long. Local visibility/zoom are not Managed state or persistence.
The Inactive overlay covers the current full canvas; navigation is locked during
gestures and target transitions.

The strip has five visual sections across fixed column units: Gain Staging,
Saturator, Shaper, Compression and Polish. Shaper is one vertical column.
Compression is a two-column section with three dials in each column and
Parallel centered in the toolbar above them. Shared MarkerControl components
combine colored device markers with headings. Input/Output subcolumns and Width
are detailed below. The state contract belongs to
[Managed Gain Staging](../../../Consolidator.Managed/Docs/Aspects/GainStaging.md).
There is no processor panel selector, adaptive grid, parameter viewport, bank
dropdown or separate detector graph.

Equalizer internal geometry: [EqualizerLayout](EqualizerLayout.md).

## Shared bank and filter interaction

Tracks/Navigation supplies the bank column, horizontal group selector and member
list as one reusable component. The vertical bank column remains at the left
edge beside the graph when the Track List is collapsed. Both bank views borrow TracksViewModel, BankCellRenderer
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

ProcessorCatalog is the single source for section IDs, widths, parameter order,
relative slots and labels; bindings derive their ordered paths from those slots.
Headers find their section by context, independently of model order. Compression
timing availability belongs to Compression/Bindings.
ChannelStripControl owns one fixed grid aligned with the Equalizer's 20-unit
inset and 1060-unit content width. Section widths are 300/150/150/300/160 units
for Gain Staging, Saturator, Shaper, Compression and Polish. All section headings
share one baseline. Each heading includes a 14-unit marker in its device color,
filled when ActivityObserver reports non-neutral processing and otherwise outlined.
Bypass changes its color to gray independently of activity. Both header and table
markers consume the same registry processor status. One click toggles device
bypass immediately; the second press of a double-click resets the device without Ctrl.
The existing local/group scope and navigation locks still apply. Separate device
B/R buttons are absent, including in the EQ header.

The toolbar contains the two Leveler buttons, Saturator Split and Compression
Parallel on one baseline. Beneath it, every processor uses the same three
112 × 92 dial rows at a 100-unit pitch. Gain Staging's input column contains
Input Level, Input Target and Width; its output column contains Output Level,
Output Target and Limit. Saturator contains Drive, Curve and Output; Shaper
contains Attack, Sustain and Output. Polish starts on the same first two rows.
There is no separate Utility row. This fixed geometry is uniformly scaled by
the editor's existing zoom. Scalar controls
retain numeric editing, Shift snapping, reset and group markers.

GainStagingBindings owns four RuntimeDialViewModel overlays for Input/Output Level/Target.
Level is editable with Leveler off; Target is editable with Leveler on. The other
dial displays telemetry. Slow gain and fast correction remain distinct. Bindings
outlive parameter controls/models; rebuild destroys controls before borrowed models.
See [RuntimeOverlay](RuntimeOverlay.md).

Track-table processor markers use six IDs: Gain Staging, Saturator, Shaper,
Compression, Equalizer and Polish. Shaper and Compression have independent
activity/bypass projections and matching observe_target contexts.
Shared/Presentation/ProcessorPresentation is the single ID/label/color catalog
for all six device headings and table markers. Named device colors also serve
EQ bank rendering, avoiding positional color indexes after device-set changes.
