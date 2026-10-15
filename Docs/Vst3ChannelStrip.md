# Fixed VST3 channel strip

## Composition and scaling

ChannelStripControl paints the five fixed processor headings and column dividers
directly from ProcessorCatalog, including before the first target snapshot.
Rebuilding parameter children does not create or replace title state. Previously
the headings were populated during Rebuild without invalidating their paint
region; they could remain absent until an unrelated parent repaint. Source review
confirmed the missing invalidation path. This correction has not been built or
verified in Live.

The editor uses two explicit fixed logical layouts: 1788 x 1104 with Bank Manager
expanded and 1212 x 1104 with it collapsed. Free window resizing and
the resize corner are disabled. A toolbar button cycles 75%, 100%, 125%,
then returns to 75%; its label displays the current percentage. Each click
applies a uniform JUCE component transform and sets the plugin window to the
matching exact size, with equal minimum and maximum size limits. A newly opened
editor starts at 100%. Zoom is editor-local presentation and is not written to
Managed state, history or persistence.

Child bounds, column count and parameter slots do not change with zoom. Mouse
events use transformed component-local coordinates, so drawing and hit testing
share the same geometry. The scale control is disabled during gestures and
target transitions, like the other navigation controls.

A 112-unit strip remains at the left edge in both layouts. Its JUCE
button displays `Hide Tracks` to collapse the Track List and `Show Tracks` to expand it. The editor
starts expanded. Collapse hides the existing table, title and bank action
controls without destroying them or changing focus/selection/subscriptions.
The processing area only translates horizontally; its size and internal
layout stay fixed. `EditorPresentation` owns this local visibility state and
reports its logical width; the host updates exact window size at the current
zoom through the same sizing path used by the scale button. The Inactive
overlay covers the current full canvas. Collapse is disabled during gestures
and target transitions, and is not persisted in Managed or host state.

The sidebar stacks Hide/Show Tracks, Undo, Redo, zoom and
Group/Local controls. The Bank Manager header shows Consolidator and a horizontal
Group/Ungroup/Clear/Reset All action row above the tracks. Its width is 576 units;
the 544-unit table starts at y = 94, with 16-unit side margins. Action button
widths are derived from the same table width and shared gaps, so the last button
stays inside the panel divider. Banks and S/M/R/B use 24-unit square cells; the six
processor slots use a 12-unit pitch with centered 8-unit markers. Large focused
processor tiles are removed. Active state colors the marker outline; effect
state colors its inner dot, with bypass dimming and accepted-feedback handling
retained. Ordinary clicks retain track focus, Ctrl-click bypasses and
Ctrl-double-click resets the processor; the hit slots use the new pitch.
B, Commit and Refine are centered in the bottom row of the Equalizer section. The processing header uses the same font/style as Consolidator
and displays the observed instance's registry label. Target switches and registry
label changes refresh it; an unavailable registry entry clears the title.

The collapse control was built and installed in Release x64 on 2026-10-14 with
exit code 0 and no warnings; artifact and installed module hashes matched.
Source review covered sizing at all three zoom settings, hidden control reuse,
gesture locking and overlay bounds. Live visual/host resizing validation remains
pending; no automated tests were run.

The subsequent sidebar/action relocation, 100% initial zoom, restored table
top border and observed-track title also compiled and installed in Release x64
on 2026-10-14 with exit code 0, no build warnings and matching module hashes.
Source review covered expanded/collapsed visibility, header label updates and
unchanged action handlers. Live visual validation remains pending.

The panel-boundary correction and compact processor markers also built and
installed in Release x64 on 2026-10-14 with exit code 0, no warnings and matching
module hashes. Source review covered button extents, marker hit slots and
unchanged focus/bypass/reset command paths. No tests or Live visual checks ran.

The wider label column, 24-unit banks/status cells, doubled Q gesture sensitivity
and accepted-value step-dial hover correction built and installed in Release
x64 on 2026-10-14 with exit code 0, no warnings and matching module hashes.
Source review covered row drawing/hit testing/scrolling, action widths, Q limits
and choice drag writes. No automated tests or Live interaction checks ran.

The Bank Manager occupies a narrow full-height left column, with its existing
single-line track rows and action buttons above them. The compact table
width removes the former large empty label area. Each row remains one horizontal
line, 24 units high with square bank cells and JUCE S/M/R/B buttons. Adjacent
status buttons share a single one-unit border. Selection
and Shift behavior retain their contracts. The processing area starts at logical
x = 688 when expanded or x = 112 when collapsed and keeps its
1100-unit width. One large analyzer with the shared source/group spectrum
sits above the channel strip in this right column. A combo box selects
Equalizer, Input Detector, Saturator Detector or Compressor Detector.
The channel strip
has five fixed columns: Input, Saturator, Compressor, Polish and Output.
Parameters run vertically in each column, following feature-owned presentation
profiles. Separate detector graphs have been removed; every processor's
controls start at the same vertical position. The shorter fixed canvas removes
the space previously reserved for these graphs.

Graph selection is local presentation: it uses the complete accepted target
cache without requesting new snapshots or changing instance/bank focus. The
same widget and model are reused. Switching retains the source/group spectrum
and cached EQ bank responses; the all-bank overlay appears only in Equalizer
mode. Selection is disabled during gestures and target transitions.

There is no processor-panel selector, adaptive grid or parameter viewport.
Bank selection is available only in the Bank Manager; the bank dropdown has
been removed. Bank selection and local/group scope address the entire strip. Clicking
a processor marker in the Bank Manager selects its instance/bank without
switching the visible interface. Separate features and ViewModels remain:
channel strip is an editor composition, not a replacement state or DSP model.

## Graphical filter editing

Equalizer frequency, gain and Q are edited only in the large analyzer. There
are no Equalizer parameter dials. Detector filters likewise have no duplicate
numeric controls. Graph handles retain dragging, Alt-drag Q and double-click
reset. Ctrl-click on a graph handle toggles its filter bypass through the same
Managed path. Bypassed handles remain visible as dim outlines at their stored
frequency/gain and remain editable and resettable. Numbered filter buttons are
removed; their former space extends the plot, with 8% top/bottom margins.
EQ processor bypass, Commit, Refine and source selection sit above the
main graph. Reset All resets the selected instance's DSP tree, using the
existing group-instance scope when group editing is selected.

The processing title is followed immediately by a 24-unit S/M/R/B strip for
the observed track. Its horizontal position follows the rendered title width,
with space reserved for the strip when long names are displayed. Table rows
and the title strip share `InstanceButtonsControl`: accepted Solo/Mute/Bypass
state, semantic colors, Shift-additive Solo and Reset action flash use the same
code and command routing. Header controls stay available with Bank Manager
collapsed and follow normal navigation locking. Separate Bank Bypass and
Reset EQ Bank toolbar controls are removed; both bank actions remain available
through Ctrl-click and Ctrl-double-click on either bank-cell view. These source
changes remain unbuilt/uninstalled pending the user's build instruction.

The analyzer's left margin contains a vertical 24-unit bank column for the
currently focused instance, centered alongside the graph. It remains available
when the main Bank Manager is collapsed. Both views share `TracksViewModel`,
`BankCellRenderer` and `BankCellInteraction`: ordinary click selects/focuses,
Shift-click/drag keeps the existing one-bank-per-instance selection rule,
Ctrl-click toggles bypass and Ctrl-double-click resets the bank. Each view has
its own gesture visited set; both repaint accepted state and temporary feedback.
Group letters/colors, focused/selected/group highlights and activity/bypass
badges are identical. Registry and target subscriptions are unchanged; the
column owns no independent selection or application state. Navigation/gesture
locking and the Inactive overlay cover both views. This source change has not
been built or installed, as requested while collecting changes.

## Snapshot and lifetime

Managed is unchanged. `TargetStateClient` requests all six existing
`observe_target` contexts for the same instance/bank, with Equalizer last.
Every reply must match its request, context, instance and bank. The client stages
the complete parameter union and publishes it only after all six snapshots
arrive. Notifications update already staged values during assembly. A target
or activation transition invalidates every outstanding snapshot identity.
This is a coherent frontend projection, not a new atomic Managed snapshot API.

One gesture session serves all scalar controls and the single graphical editor.
During a gesture only its originating scalar or graphical editor stays enabled.
History ownership, final acknowledgement, group delta writes and Leveler
runtime presentation retain their existing contracts. Controls are destroyed
before replacing their borrowed parameter models. The analyzer control is
destroyed before its binding-owned model.

Reusable history/gesture sessions and parameter intents live in
`Source/Shared/Editing`; `StateValueViewModel` lives in `Shared/Presentation`.
Dial no longer depends on the Parameter component. The host owns fixed canvas
composition; features own parameter ordering and choices.

## Stable parameter presentation

Successful Reset, Undo/Redo, table edits, Ungroup, Clear and Commit/Refine
commands release their input lock without clearing graphs or recreating
parameter controls. Accepted values and effective metadata update existing
models through the ordered notification stream. Their completion callbacks
must match the target generation and active session. Target changes, activation
and rejected commands still obtain a complete snapshot.

Input locking during another control's gesture no longer dims parameter rings,
Character indicators or macro toggles. The accepted value, group scope and
Leveler automation determine presentation; editability still blocks input.
This removes the whole-strip brightness changes previously caused by each
begin/end. Intentional action flashes and actual bypass/value changes remain.

## Validation status

The implementation has undergone direct source/contract review. On 2026-10-14,
the current editor including the left Bank Manager and 100/125/150/200% scale
button compiled successfully in Release x64 with exit code 0 and no warnings
in the build output. The command, after entering Developer PowerShell, was
`cmake --build Consolidator.Vst3/build --config Release --target ConsolidatorVST3Install --parallel 4`.
The installed bundle in `C:/Program Files/Common Files/VST3` was updated and
its module SHA-256 matched the build artifact. Existing published Managed
runtime was reused; Managed was not built or published. No tests or Live
validation were performed for this layout.
Required host validation covers scaling/hit testing, all six loaded contexts,
detector and EQ gestures, bypass restoration, history, groups, Leveler and
project reload. Earlier host results apply to the previous editor.

The subsequent 75/100/125% presets, removed bank dropdown and stable command/
parameter presentation compiled and installed successfully on 2026-10-14.
The Release x64 build exited with code 0 and no warnings; the installed module
hash matched the artifact. Managed runtime was reused. Source review confirmed
the previous clear/recreate and transient parameter-dimming paths; disappearance
of the reported visual flicker still requires a new Live pass.

After the Visual Studio update, CMake configuration required the exact existing
instance version:
`-DCMAKE_GENERATOR_INSTANCE=C:/Program Files/Microsoft Visual Studio/18/Community,version=18.11.12306.390`.
Configuration and build used the absolute `cmake.exe` path from the Visual
Studio CMake installation. No project-side compatibility path was introduced.

The single-analyzer selector, removed detector widgets and compact fixed
canvas subsequently compiled and installed successfully in Release x64 on
2026-10-14, with exit code 0 and no warnings in the build output. Installed
module hash matched the artifact. Managed runtime was reused. A Live pass
for all four graph modes, switching, retained spectra/overlay and edits remains
pending; compilation is not visual or interaction validation.

## Equalizer feature ownership and layout

EqualizerControl owns a fixed 1060 x 338 section at the existing processing origin.
The 310-unit body contains a 32-unit heading followed by 278-unit aligned
lists/graph; a 28-unit action row immediately follows the body. From left to
right the body contains contiguous columns: 25-unit BankColumnControl (24-unit
square cells plus its border), 136-unit GroupTrackList, 787-unit AnalyzerControl
and 112-unit AnalyzerSourceList. There are no gaps between the columns. Banks
start at the left edge under the heading; the analyzer consumes all remaining
body width. The EQUALIZER heading uses the same centered 16-unit Arial style as
processor headings. The plot has equal 8-unit horizontal margins and no vertical
margins, so its grid spans exactly the group-list height.

The group list is a JUCE ListBox projection of the focused bank's
registry group. Each member track appears once, in registry order; an ungrouped
bank shows its own track. Local/Group editing scope does not change membership.
Long names have full-name tooltips; a narrow scrollbar handles many members.
Refreshing topology/focus/labels updates the list without changing target
selection, protocol subscriptions or history. Clicking a member emits the existing
SelectBank intent for that track and its bank in the focused group. The existing
target/snapshot route performs the switch; current focus drives the row highlight.
Member navigation is disabled during gestures, target transitions and inactivity.
Programmatic row refresh cannot trigger navigation.

The source JUCE ListBox replaces the combo box. Its four rows show Equalizer,
Input, Saturator and Compressor; tooltips identify the detector sources. Exactly
one source is selected, initially Equalizer. The selected row uses an 8-unit
filled active square and other rows an outlined dim square, matching track markers.
The existing snapshot/context switch and gesture/transition navigation lock are
retained. No independent detector widgets are introduced.

B (Equalizer Bypass), Commit and Refine are centered as one contiguous 204-unit
button strip under the full body. Their 28/88/88-unit widths and connected edges
preserve square JUCE styling and single shared borders. B follows accepted
state; optimization commands and their eligibility remain unchanged.

Shared track/bank selection, rendering and click behavior stay in Tracks and are
borrowed by the vertical column. Native DSP and persistence remain unchanged. Managed capture and the FFT
notification now carry the reference pair, as documented below. Source review covered layout bounds, list/model lifetime, group
membership, initial source selection without construction callbacks, source
switching, accepted state and edit locks. These changes have not been built,
installed or verified in Live.
Below the four source rows is a second contiguous marker list with Reference,
Group and All Banks. These independent visibility toggles use the same square
marker row renderer as source selection. Reference controls the optional stereo
VST3 sidechain spectrum, Group controls the existing group spectrum and All Banks
controls the summed EQ response. All start visible and remain editor-local; they
do not change analysis demand, parameters, history or audio. Overlay controls
follow normal navigation locks. Their data/caches update even while hidden.
Reference now follows main audio through Managed capture and worker FFT; the
common fft notification carries all spectra. See Vst3SidechainReference.md for
the transport/threading contract. A matching Managed runtime rebuild is required.
This subsequent layout/reference change is source-reviewed only, not built or
verified in Live.