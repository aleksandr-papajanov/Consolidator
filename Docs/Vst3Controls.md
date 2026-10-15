# JUCE controls and Input runtime presentation

## Source mapping and ownership

`DialControl` ports Max `DialControlCore`, `DialRenderer` and `DialValueEditor`.
`ParameterControl` composes the numeric dial, boolean button or reusable
choice control (used for Compressor Character). Feature presentation metadata
provides the choice labels. `ParameterEntry` contains accepted UI values and
physical/effective range metadata; it is shared with the analyzer without
depending on a widget header. The interim `ParameterRow` slider implementation
has been removed from the source target and filesystem.

Controls emit intents through `Shared/Controls/Parameter/Bindings/ParameterBindings` and
`Features/Equalizer/Bindings/AnalyzerBindings`. `Shared/Clients/StateCommands`
and feature controllers send the existing commands;
`Shared/Editing/DialGestureSession` owns continuous
history transactions. Controls do not
apply DSP defaults, persist values, propagate groups or mutate Managed state.
Bindings own the parameter/analyzer presentation models, not JUCE Components;
`EditorPresentation` creates controls and panels that borrow those models.
`Features/ChannelStrip/Controls/ChannelStripControl` lays out controls in fixed vertical
processor columns. The scale button uniformly scales the complete editor canvas
at 75%, 100% or 125%; free window resizing is disabled.
Each column feature's `Presentation/ParameterPresentation.h` contains
display metadata only: parameter order and choice labels. All columns use the
same fixed control slots beneath the shared analyzer selector.
Feature controllers own discrete-edit history policy; Input bindings own the
runtime Leveler model and connect it to the reusable dial overlay contract.
The shared controls remain context-agnostic.
Macro order follows the feature profiles. Filter frequency/gain/Q and bypass
editing lives in the graphical analyzers, with no duplicate filter dials.
See [Vst3ChannelStrip.md](Vst3ChannelStrip.md) for the current layout and validation status.
The source folders and namespaces are documented in
[Vst3Architecture.md](Vst3Architecture.md).

## Dial behavior

Buttons and ComboBox share the editor's JUCE theme and 28-unit control height.
Ordinary control labels use Arial 12; headings use the theme's separate sizes.
Behavior subclasses no longer implement button painting. This includes
Bank Manager S/M/R/B and analyzer filter buttons; see [Vst3Theme.md](Vst3Theme.md).

Dial geometry follows the Max controls: the same 240-degree sweep, relative
vertical drag sensitivity of 0.007 normalized units per pixel, three-pixel
outer ring and label placement. The temporary value label returns to its
parameter name after 500 ms. Macro values display as percentages; gain,
level, target and output display in dB, following the current Max panels.

The ring maps the full physical range. Group scope draws only the Managed
effective interval on that same geometry and uses the bank group color.
Zero-span effective intervals cannot be edited. Dials normally edit continuously.
Holding Shift during a dial drag temporarily snaps dB values to 3 dB and scalar
values to 0.1. Rounding
matches JavaScript ties toward positive infinity, including negative dB.
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
Macro toggles and Character retain the actual installer's frontend history
begin/write/end behavior. Ordinary writes use that transaction ID. Pending
discrete writes are canceled if target generation, scope or active session
changes before begin acknowledgement. Toggle double-click invokes reset;
Leveler reset uses dedicated disable. Successful notifications update models without recreating
the panel between individual drag changes; a rejection refreshes the target.

## Input Leveler

The accepted `input_gain.leveler` flag controls editability of the Level dial.
While enabled, its outer ring displays `input_gain.level` from the existing
`dsp_telemetry` frame and is read-only. The separate fast correction
`input_gain.level_fast` draws a smaller relative arc centered at zero over
the existing -6/+6 dB display range. It does not move the outer knob.

Telemetry is accepted only for the current Input target and a newer revision,
after its snapshot is ready and audio is prepared. Both values are validated
before updating the presentation. Target/audio-configuration transitions clear
the runtime display; disabling Leveler hides the inner arc and restores the
accepted static level. A Leveler enable notification cancels a manual level
gesture. The displayed runtime pair is separate from `ParameterEntry` and is
never submitted through `write`, history or persistence.

Disabling Leveler still invokes `disable_input_leveler`, allowing Managed to
commit each grouped instance's own slow gain with its flag. The frontend does
not copy the currently displayed runtime gain to peers.

## Validation

The earlier Live pass exercised EQ dragging and basic Input Leveler enable,
read-only Level and disable behavior without audio playback. Nonzero runtime
fast correction and grouped controls were not verified.

Source/contract review and Release x64 compilation of the reorganized frontend
succeeded. Its current bindings, focus and overlay have not been verified in
Live. See [Vst3Architecture.md](Vst3Architecture.md) for current ownership and
validation status, and [Vst3Migration.md](Vst3Migration.md) for packaging.
Shared StateValue/Parameter/Dial view models own accepted metadata, formatting,
ranges, precision and preview; the Input feature view model owns its runtime
leveler presentation. Controls keep pointer and paint state. The old/new review,
including corrected discrete history and reset behavior, is in
[Vst3ParityReview.md](Vst3ParityReview.md).
