# JUCE editor migration

## Reuse boundary

The existing Max UI is the behavior specification for the JUCE frontend, not
an abandoned prototype. Managed, Native DSP, C ABI, protocol commands, state
definitions, peer propagation, topology policies, history storage, optimizer
and persistence schema are reused unchanged. The initial parameter list is a
frontend integration surface; it is not a replacement design for the finished
Consolidator interface. It has now been replaced by native controls and the
graphical analyzer. The current fixed, uniformly scaled channel strip shows
all processors simultaneously; see [Vst3ChannelStrip.md](Vst3ChannelStrip.md).
Visual and host validation of this composition remain pending.

| Existing Max source | Native frontend implementation |
| --- | --- |
| `Shared/Editing/DialGestureSession.js` | `Source/Shared/Editing/DialGestureSession.*` |
| `Shared/Controls/Dial/DialControlCore.js`, `DialRenderer.js`, `DialValueEditor.js`, Input runtime view models | `Source/Shared/Controls/Dial/DialControl.*`, `ParameterControl.*`, `Source/Shared/Controls/Parameter/Bindings/ParameterBindings.*`; [Vst3Controls.md](Vst3Controls.md) |
| `Shared/Clients/RegistryClient.js`, `RegistrySnapshotAssembler.js`, `RegistryDeltaApplier.js` | `Source/Shared/Clients/RegistryClient.*` |
| `Features/BankManager/Controls/BankManagerLayout.js`, `BankManagerInteraction.js`, `BankManagerHitActions.js`, `ViewModels/BankManagerRows.js` | `Source/Features/Tracks/Controls/TrackListControl.*`, `ViewModels/TracksViewModel.*` |
| `Features/BankManager/Controllers/BankManagerTopologyEditor.js` | `Source/Features/Tracks/Controllers/GroupEditor.*`, `TrackController.*` |
| `Shared/Clients/TargetStateClient.js` relative-path/write-mode contract | `Source/Shared/Clients/TargetStateClient.*`, `StateCommands.*`, `Source/Features/Equalizer/Bindings/AnalyzerBindings.*`, `Source/Shared/Protocol/ProtocolFrame.cpp` |
| `Shared/Theme/UiColors.js` | `Source/Shared/Theme/UiColors.h`, preserving the existing palette |
| Context-specific parameter order and choice labels | `Source/Features/ChannelStrip/Presentation/ProcessorCatalog.h`; five section declarations, with graphical-only Equalizer editing |
| Discrete edit-history policy and Input runtime overlay | Section history paths in `ProcessorCatalog`, `Features/ChannelStrip/Bindings/ChannelStripBindings.*`, `Features/ChannelStrip/Input/Bindings/InputBindings.*` and `Input/ViewModels/InputLevelerViewModel.*`; shared controls consume only the generic dial overlay contract |

Paths above are relative to `Consolidator.Vst3`. The final layers, namespaces
and ownership are described in [Vst3Architecture.md](Vst3Architecture.md).

Only presentation caches and UI intents live in these C++ classes. They do not
implement Managed command handling, DSP algorithms, group propagation or a
second application-state store.

## Continuous gestures

Pointer down opens one Managed history point. Moves received before its
acknowledgement retain only the latest pending value; release before the
acknowledgement retains the final position and pending end. Once ready, a
reused 33 ms JUCE timer submits the latest dirty value through the existing
coalescing transaction contract. Successful callbackless intermediate writes
do not create pending response entries.

Release submits the final transaction write, then an acknowledged transaction
zero write, then closes history. Notifications precede the final reply in the
lossless FIFO. Parameter controls retain their optimistic preview until that
reply; acknowledged updates change their authoritative presentation cache
without moving the active drag backward. Rejection clears preview and requests
the current target snapshot. Closing a window queues its final position and
history end before detaching its listener; no reply retains a destroyed editor.

Bank, scope and topology navigation are disabled during a gesture.
Only its active control remains enabled, and it is disabled while awaiting
the final acknowledgement. Group float edits use `delta`, preserving peer
offsets, and clamp to Managed's effective range. Local floats and discrete
values use `copy`. Effective-range notifications update the same parameter
presentation; there are no frontend DSP defaults.

EQ snapshot paths use `equalizer.filter.N...`, while writes use
`equalizer.bank.filter.N...` to select the observed bank. This is the existing
Max `TargetStateClient.relativePath` conversion, not a compatibility alias.

## Bank Manager

One full registry request initializes a staged presentation snapshot. It is
published only after its matching `registry_done`, complete instance/bank/group
counts and consistent received membership frames. Revisioned deltas update the current
cache directly. A revision gap requests one replacement snapshot; an older
replacement cannot overwrite a newer revision. Processor-marker batches and
latest track-level telemetry do not advance topology revision or refetch rows.

The table preserves the existing six processor markers, seven bank cells,
group letters/colors, activity dots and instance `S/M/R/B` controls. The output
level dot uses the existing nonlinear `-60..0 dBFS` mapping. Scrolling and
selection remain local to the control. Shift-click/Shift-drag toggles ungrouped
banks, visits each crossed cell once and cannot select a second bank from the
same instance. Selecting a new grouped bank selects group scope; selecting the
already focused bank preserves a manually chosen local scope.
Every ordinary bank click clears previous explicit selections first, including
clicks on grouped or already focused banks. Only an ungrouped bank is then
explicitly selected. Shift selection does not change focus or clear unrelated
selections. Processor navigation to another instance also clears selection.

Group creation chooses the first unused user group `1..15`, matching the
existing sixteen-color UI limit. Automatic group `0` remains immutable. Group
and Ungroup issue acknowledged explicit-address topology writes inside one
history point. After begin is acknowledged, all writes and the history end
are queued together, as in Max, so closing after write submission does not
interrupt the remaining writes. Closing while begin is still pending cancels
the unsent action and queues history end after its begin; removing the editor's
callbacks cannot leave an open reservation. As in the existing command surface, separate instance writes
are not a cross-instance atomic command. Completion waits for all replies and
reports any rejected write; confirmed Managed changes remain available to the
existing Undo operation. The frontend does not invent rollback state.
Clear uses the existing coordinator command, with an asynchronous confirmation,
and preserves automatic group A.

Processor/bank Ctrl-click toggles bypass; Ctrl-double-click requests targeted
reset. Solo is exclusive on normal click and additive with Shift; mute and
instance bypass are additive. Their scope follows the original focused-instance
rule. Instance Reset retains the existing current-target `dsp` reset behavior.
Commit and Refine invoke the existing Managed operations, including their own
history advancement after successful approximation.

History ownership is preserved: gestures and topology edits open a frontend
history point; Reset, Commit and Refine advance history in their Managed
handlers and receive no additional frontend point. The actual Max
`UiBindingInstaller` passes TransactionClient to macro toggles and Character;
those value changes open a frontend point and retain its transaction ID in
ordinary writes. Toggle double-click reset does not open another point.
Leveler reset invokes dedicated disable, preserving each instance's slow gain.
Instance mute/solo/bypass and table bypass controls remain direct commands.

## Viewer lifecycle

Managed still has one active viewer. The JUCE frontend mirrors that contract
on its message thread: activating another window disables the previous
window's editing, submits its final gesture writes and clears preview. Foreground
focus selects an existing window again. There is no Activate action and no independent Managed viewer.
Activation invalidates a pending registry assembly, obtains current registry
and target snapshots and resumes editing after the matching target snapshot.
Deleting the observed instance cancels its gesture and returns focus to the
editor's own instance.

Windows editor focus now selects the active viewer automatically. A 30 Hz
message-thread timer compares the editor peer's root HWND with the foreground
window, including focus on the host's floating-window title bar. Moving between
controls does not reconnect the viewer. The last viewer remains active while
focus is outside plugin editors, including transient menus. Focusing another
editor finishes the previous gesture before queuing activation. Native ownership
is claimed before sending the request; replies must match both that owner and
its activation generation. This prevents delayed acknowledgements from taking
ownership back. There is no Activate button. After an activation or presentation
error, leaving and refocusing the editor requests reconnection. This focus change
was compiled successfully in Release x64 on 2026-10-13; focus switching has
not yet been verified in Ableton.

An unfocused or inactive editor displays a full-window black overlay at 65%
opacity with centered `Inactive` text. It intercepts pointer and keyboard
input; the first click focuses the window without editing a covered control.
The overlay stays visible until the window is focused and Managed activation
has been acknowledged. Ordinary busy operations in the active window use the
existing disabled controls. The overlay is presentation only and is not persisted.
The overlay was included in that successful Release build; its visual and input
behavior remains unverified in Ableton.

## Analyzer response calculation

`AnalyzerCurveCalculator` ports the presentation formulas from Max
`BiquadCalculator` and `AnalyzerCurveCalculator`: gain, tilt, low/high shelf
and bell responses, with 1024 logarithmic frequency points from 20 to 20000 Hz.
Coefficients are calculated once per filter. Bypassed filters retain their
stored response shape and draw in the same gray as their markers; the combined response includes only active
filters when the bank is active. Plot coordinates use the existing -24/+24 dB
scale and remain unclamped for renderer clipping.

This is frontend presentation code, not a replacement for Native DSP. Its
caller must provide a prepared positive sample rate and valid authoritative
bindings, including positive frequency and Q. It contains no state defaults,
protocol requests or audio-thread work.

`AnalyzerControl` is connected to accepted target snapshots and state changes
for Equalizer and the Input, Saturator and Compressor detectors. It retains
only relevant presentation bindings. Native filter definitions mirror the
existing JS catalog; fixed Q is presentation metadata, while editable values
and physical/effective ranges come from Managed. The view is cleared during
target transitions and rejects FFT frames from another target instance.

The source and group spectra use the unchanged `fft` frame (1024 samples,
513 bins per spectrum). Smoothing coefficients remain 0.5 for attack and 0.16
for release. Spectra use a separate logarithmic grid based on logical plot
width, retaining the loudest interpolated/bin value within each column.
Finite values outside the visible vertical range are clipped to the plot instead
of breaking the curve. These intentional rendering improvements over Max and
the independent geometry caches are described in [Vst3AnalyzerRendering.md](Vst3AnalyzerRendering.md).
The peer spectrum uses the bank's group color regardless of edit
scope. Reference frequency lines, filter colors and plot margins match Max.

The processor publishes its prepared sample rate through an atomic control
value; its existing message-thread timer emits the local editor event
`audio_configuration_changed` only when that value changes. This is not a
Managed protocol extension. No synthetic 48 kHz default is introduced. Before
audio preparation the analyzer remains blank; release/rate changes discard
old spectra. The unchanged FFT protocol carries no remote source sample rate,
so remote focus uses the current plugin's host sample rate, as the existing
frontend does. This depends on instances sharing the host audio configuration
and remains a host-validation item.

Filter handles support frequency/gain dragging, Alt-drag Q and double-click
reset. Frequency, gain and Q gestures are continuous within effective limits.
The analyzer has no Rough/Precise state, modifier-based snapping or Rough-grid
dots. Frequency uses the logarithmic plot axis; Alt-drag Q uses a linear
increment of 0.02 Q per logical vertical pixel. Fixed frequency reference lines
remain. Temporary Shift snapping is exclusive to native dials;
see [Vst3Controls.md](Vst3Controls.md). Ctrl-click toggles filter bypass without
starting a drag/history gesture. Bypassed filter handles remain visible as dim
outlines and keep the same hit region, dragging and double-click reset behavior.
Separate numbered filter buttons are removed.

`DialGestureSession` now retains a batch of typed `StateWriteEntry` values.
Scalar dials use one entry; analyzer frequency/gain changes use one existing
Managed `write` command containing both entries. Q changes join that gesture's
latest batch. The same begin acknowledgement, 33 ms coalescing, final
transaction-zero acknowledgement, history end and editor-close behavior serve
both controls. There is no second gesture transport. Group floats retain
`delta` mode and authoritative effective limits. Local preview survives
intermediate notifications and is cleared after the final acknowledgement or
rejection; filter-row gestures also update the displayed curve immediately.
Filter reset invokes the existing Managed reset command, which owns history.

The Equalizer also requests the existing `observe_banks` snapshot and displays
the combined response of all seven banks in the original overlay color. One
request may be outstanding; further EQ notifications mark it dirty and trigger
one fresh request after the pending reply, rather than issuing a request per
entry in a position write. Replies must match the current request, target and
context; changing target invalidates them. All seven distinct banks are parsed
before replacing the presentation cache. Focused and all-bank curves use the
same filter reader and response calculator. The overlay combines accepted
responses of the other six banks with the focused bank's current accepted/preview
response, so both curves use the same gesture position. The focused bank index
comes from the authoritative target binding; its contribution in a delayed
bank snapshot is excluded. Its
summed coordinates remain unclamped, with strokes clipped by the plot rectangle
instead of flattening out-of-range peaks; this intentionally corrects the old
Max overlay behavior. It is absent when every bank is bypassed.

## Remaining work and validation

Native dials, temporary Shift snapping and Input runtime overlays are implemented.
The earlier Live pass exercised loading, EQ dragging, Undo/Redo, saving with
the editor closed, reopening the editor and basic Leveler transitions. It did
not verify complete Live Set reload, cross-instance groups or cutover.

The frontend was subsequently reorganized into clients, bindings, session,
feature controllers and host composition. Source/contract review and Release
x64 compilation succeeded, and the installed bundle was updated. That earlier
Live pass does not validate this refactor; its focus, overlay, gestures, groups
and persistence behavior still requires a new host pass. No unit/integration
tests were run for the refactor. See [Vst3Architecture.md](Vst3Architecture.md)
and [Vst3Migration.md](Vst3Migration.md).
Explicit view-model ownership and the subsequent old/new source review are
described in [Vst3ParityReview.md](Vst3ParityReview.md).
