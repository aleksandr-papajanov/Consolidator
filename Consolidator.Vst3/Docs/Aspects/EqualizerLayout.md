# Equalizer layout

EqualizerControl owns a fixed 1060 × 338 section. Its 310-unit body has a 32-unit
heading and 278-unit aligned content, followed by a 28-unit action row. Contiguous
columns from the left: BankColumnControl 25 (24 plus border), GroupTrackList 136,
AnalyzerControl 787 and AnalyzerSourceList 112. The plot uses equal 8-unit
horizontal margins and no vertical margin, matching group-list height.
EQUALIZER uses the same centered 16-unit Arial heading as other devices.

The group JUCE ListBox projects the focused bank's registry group, one track per
member in registry order; an ungrouped bank shows itself. Scope does not change
membership. Clicking a row emits SelectBank for that member's group bank through
the existing target/snapshot route. Refreshing rows never emits navigation.
Long names have tooltips; many members use a narrow scrollbar.

The source ListBox selects Equalizer or the Gain Staging/Saturator/Compressor
detector from the already accepted target cache. Selected source uses an active
square, others an outlined square. Switching does not change track/bank focus
or request another snapshot. It reuses the same graph and retains spectra/cache;
all-bank response is shown only for Equalizer. Selection is navigation-locked.

Below sources, Reference/Group/All Banks independently control visibility through
the same square-marker renderer. All start visible, remain editor-local, keep
receiving cache updates while hidden and do not alter audio/history/analysis demand.
Reference follows sidechain through [Managed capture/FFT](SidechainReference.md).

B, Commit and Refine form a centered contiguous 204-unit strip below the body,
with widths 28/88/88 and single shared square borders. B displays accepted EQ
processor bypass. Bank bypass/reset remain Ctrl-click/Ctrl-double-click actions
on either bank-cell view; separate toolbar buttons are absent.
