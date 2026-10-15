# JUCE controls

## Source mapping and ownership

`DialControl` owns pointer interaction and drawing; DialViewModel owns presentation values.
`ParameterControl` composes the numeric dial, boolean button or reusable
choice control (used for Compressor Character). Feature presentation metadata
provides the choice labels. `ParameterEntry` contains accepted UI values and
physical/effective range metadata; it is shared with the analyzer without
depending on a widget header. The interim `ParameterRow` slider implementation
has been removed from the source target and filesystem.

Controls emit intents through `Shared/Controls/Parameter/Bindings/ParameterBindings` and
`Features/Equalizer/Bindings/AnalyzerBindings`. `Shared/Clients/StateCommands`
and feature controllers send the existing commands;
`Shared/Editing/DialGestureSession` delivers continuous gesture boundaries
and values. Managed owns history policy. Controls do not
apply DSP defaults, persist values, propagate groups or mutate Managed state.
Bindings own the parameter/analyzer presentation models, not JUCE Components;
`EditorPresentation` creates controls and panels that borrow those models.
`Features/ChannelStrip/Controls/ChannelStripControl` lays out controls in fixed vertical
processor columns. The scale button uniformly scales the complete editor canvas
at 75%, 100% or 125%; free window resizing is disabled.
ChannelStrip's `Presentation/ProcessorCatalog.h` contains display metadata only:
section/parameter order and choice labels. All columns use fixed control slots.
Managed owns discrete-edit history policy; Gain Staging bindings own the
runtime Leveler model and connect it to the reusable dial overlay contract.
The shared controls remain context-agnostic.
Macro order follows the feature profiles. Filter frequency/gain/Q and bypass
editing lives in the graphical analyzers, with no duplicate filter dials.
See [Vst3ChannelStrip.md](ChannelStrip.md) for the current layout and validation status.
The source folders and namespaces are documented in
[Vst3Architecture.md](FrontendComposition.md).

## Dial behavior

Buttons and ComboBox share the editor's JUCE theme and 28-unit control height.
Ordinary control labels use Arial 12; headings use the theme's separate sizes.
Behavior subclasses no longer implement button painting. This includes
track S/M/R/B buttons and analyzer action buttons; see [Vst3Theme.md](Theme.md).

Dial geometry uses a 240-degree sweep, relative
vertical drag sensitivity of 0.007 normalized units per pixel, three-pixel
outer ring and label placement. The temporary value label returns to its
parameter name after 500 ms. Macro values display as percentages; gain,
level, target and output display in dB, according to parameter units.

The ring maps the full physical range. Group scope draws only the Managed
effective interval on that same geometry and uses the bank group color.
Zero-span effective intervals cannot be edited. Dials normally edit continuously.
Holding Shift during a dial drag temporarily snaps dB values to 3 dB and scalar
values to 0.1. Rounding
uses ties toward positive infinity, including negative dB.
The unsnapped gesture position is retained, so small pointer movements can
accumulate across snap thresholds. The modifier is read on each drag event;
pressing/releasing Shift applies/removes snapping without starting a new history
point or rebasing the raw position. There is no global precision button/state,
and analyzer gestures never use this option. Discrete step dials keep their
own fixed choices.

Accepted notifications update metadata and committed display values while
preserving the active preview. Release retains that preview until the final
acknowledgement and history end. Rejection clears preview and the temporary
value label. Double-click requests the existing Managed reset command rather
than a frontend default. All widget timers stop during destruction.

Character retains the existing three-position indicator and
vertical selection gesture with sensitivity 0.015 times the two intervals.
Pointer hover does not change its indicator or displayed value. During a left
drag, a separate drag choice previews the snapped position; otherwise drawing
uses the accepted model choice. Each changed drag step emits a discrete write,
including a reversal to a value whose earlier notification is still pending.
Mouse-up clears the drag preview and restores the accepted presentation.
Macro toggles and Character submit ordinary discrete state edits. Managed
decides history from the changed registered values; frontend history catalogs
and begin/end wrappers for discrete edits are absent. Toggle double-click
submits reset; Managed owns Leveler reset/freeze behavior. Successful
notifications update existing models; rejection refreshes the target.


Runtime overlay: [RuntimeOverlay](RuntimeOverlay.md).

## Validation

Current lifecycle/diagnostic scenarios are in [Validation](Validation.md). Report actual source, build and host evidence for the changed revision; earlier host passes do not validate later changes.
