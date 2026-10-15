# Equalizer layout

EqualizerControl owns a fixed 1060 × 338 section with a 32-unit heading,
24-unit horizontal group row and 282-unit aligned content below both. Contiguous
columns from the left: Tracks navigation BankColumnControl 25 (24 plus border),
GroupTrackList 136, AnalyzerControl 787 and AnalyzerSourceList 112. The plot uses equal 8-unit
horizontal margins and no vertical margin, matching group-list height.
EQUALIZER uses the same centered 16-unit Arial marker heading as other devices.

Tracks/Navigation/Controls/TrackNavigationControl owns the bank column, group
selector, member list and temporary group browsing state. Equalizer places this
reusable component and uses its content width to align the graph. The navigation
container intercepts input only through its children, leaving the graph interactive.
GroupSelectorControl starts to the right of the vertical bank column and shows
all sixteen groups A–P in adjoining 24-unit JUCE buttons. Existing groups have
their group color; the browsed group has a filled color background. Empty groups
are dimmed and cannot be selected. Tooltips show member counts and identify
automatic group A. Membership comes from the accepted registry via
TracksViewModel.GroupSizes/GroupTracks; the controls do not maintain another topology.

Clicking a group changes only the ListBox's browsing context. It does not send
commands, select an EQ target, change a bank or alter edit scope. This browsing
choice is tied to the track and bank focused when the group was chosen. Changing
either returns the list to the newly focused bank's group; an ungrouped bank
shows only its own track. If the browsed group disappears, browsing also returns
to the focused bank's group. Parameter updates and editing locks preserve browsing.
The JUCE ListBox projects one track per member in registry order and scrolls to
the top when its displayed group changes. Clicking a row emits SelectBank for
that member's exact bank through the existing target/snapshot route. Focus
highlighting matches both instance and bank. Refreshing rows never emits navigation.
Long names have tooltips; many members use a narrow scrollbar.
Each name is preceded by the same output-level dot as Bank Manager, drawn by
TrackLevelRenderer from TracksViewModel.OutputAlphaFor. Accepted track-level
telemetry repaints both views without rebuilding group membership or changing focus.

The five source marker rows select Equalizer, Input detector, Saturator detector,
Compression detector or Output detector from the already accepted target cache.
Input/Output use independent gain_staging.input_detector/output_detector paths. Shared MarkerRowControl supplies
input and presentation for source and overlay rows. Source selection remains
exclusive; overlays remain independent. Selected source uses an active square,
others an outlined square. Switching does not change track/bank focus
or request another snapshot. It reuses the same graph and retains spectra/cache;
all-bank response is shown only for Equalizer. Selection is navigation-locked.

Below sources, Reference/Group/All Banks independently control visibility through
the same square-marker renderer. All start visible, remain editor-local, keep
receiving cache updates while hidden and do not alter audio/history/analysis demand.
Reference follows sidechain through [Managed capture/FFT](SidechainReference.md).

Commit and Refine are stacked at the bottom of the right source/overlay column,
each 88 × 28 with a 4-unit gap and 16-unit bottom inset. EqualizerController
sends their existing Managed commands with the same completion and editing locks.
The colored marker beside EQUALIZER handles processor bypass on click and reset
on double-click without Ctrl.
Bank bypass/reset remain Ctrl-click/Ctrl-double-click actions
on either bank-cell view; separate toolbar buttons are absent.
