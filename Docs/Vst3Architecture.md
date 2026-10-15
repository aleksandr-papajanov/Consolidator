# VST3 frontend architecture

## Boundary

Managed remains the single owner of committed values, topology, shared history,
main/group/reference analysis, optimization and persistence. Native DSP is reused
unchanged. The frontend owns protocol clients, presentation caches, bindings,
gestures and JUCE widgets. All frontend clients/bindings run on the message thread;
audio processing does not depend on them. No compatibility paths are introduced.

## Feature-first source organization

Paths below are relative to `Consolidator.Vst3/Source`; namespaces are relative
to `consolidator::vst3`.

| Folder / namespace | Responsibility |
| --- | --- |
| `Host/Plugin` / `host::plugin` | JUCE processor/editor, ABI/runtime wiring, host audio and persistence callbacks |
| `Host/Editor` / `host::editor` | Composition, activation/focus, event routing, sidebar and pure EditorLayout geometry |
| `Features/Tracks` / `features::tracks` | Desired UI target/scope, track/bank/group selection, topology commands, registry/level presentation and reusable track controls |
| `Features/ChannelStrip` / `features::channelstrip` | One five-column processor view, binding composition and section/history metadata |
| `Features/ChannelStrip/Input` / `features::channelstrip::input` | Leveler telemetry model, runtime-overlay binding and edit restrictions |
| `Features/Equalizer` / `features::equalizer` | Complete EQ section: analyzer, source selector, EQ Bypass, Commit/Refine, vertical banks, response/FFT presentation and graph gestures |
| `Shared/Clients` / `shared::clients` | Typed protocol clients and accepted projections, with no widget/feature dependencies |
| `Shared/Protocol` / `shared::protocol` | Owned frames, strict wire decoding and command value types |
| `Shared/Editing` / `shared::editing` | Generic gesture/history mechanism and parameter intents |
| `Shared/Controls/Dial` / `shared::controls::dial` | Generic dial widget/model and runtime-overlay contract |
| `Shared/Controls/Parameter` / `shared::controls::parameter` | Generic parameter controls/bindings, value models and section metadata contract |
| `Shared/Presentation` / `shared::presentation` | Accepted value metadata shared by parameter/dial presentation |
| `Shared/Controls`, `Shared/Theme` | Action buttons, inactive overlay, reusable colors/metrics and editor-local LookAndFeel |

Feature ownership follows behavior rather than screen location. Tracks owns
TrackListControl, InstanceButtonsControl, BankCellRenderer and BankCellInteraction.
EqualizerControl owns AnalyzerControl, BankColumnControl, GroupTrackList, AnalyzerSourceList and the EQ action
controls. The vertical banks borrow the same TracksViewModel and common bank
rendering/interaction as the table; there is no second selection/registry owner.
The processing title borrows InstanceButtonsControl from Tracks.
Equalizer owns its internal fixed geometry; EditorLayout places the entire section
as one rectangle. The host routes protocol streams into its graph and connects
existing command callbacks, while the section controls accepted bypass, scope,
source selection and child editing availability.
TracksViewModel is the only desired UI track/bank/scope context. TargetStateClient
separately correlates requested/accepted protocol contexts and generations.
EditorPresentation has no duplicate selected-track/bank/scope fields.

ChannelStrip owns its fixed layout and child construction in ChannelStripControl.
ChannelStripBindings composes generic ParameterBindings with the Input module.
ProcessorCatalog declares the five sections, order, Character choice labels and
exact discrete-history paths. The former top-level processor profiles and tiny
ParameterHistoryController classes are removed. Processor-specific behavior is
retained; no per-processor copies of parameter controls/models are introduced.
Shared metadata carries history-path declarations as data; only the feature/host
uses them to select the existing history mechanism.

Shared remains organized by reusable component. Generic bindings own value models,
not Component instances. Controls emit typed intents rather than commands.
Clients never include widget/editor/feature headers. Protocol cannot depend on
host, clients or presentation. EditorLayout consumes only visibility/text metrics
and computes fixed rectangles; it does not inspect state or issue commands.
EditorPresentation applies geometry and connects existing clients, feature
bindings, views and controller actions. Managed remains the committed-state owner.

The refactor contract and validation constraint are in
[Vst3FeatureRefactor.md](Vst3FeatureRefactor.md).

## JUCE differences from the Max frontend

- `AudioProcessor` owns per-plugin audio/runtime/Managed registration; the
  `AudioProcessorEditor` and its child `Component`s are disposable views.
  Presentation clients and controls therefore live with `EditorPresentation`,
  not the processor or a persistent Max-style UI instance.
- JUCE owns host audio callbacks and component lifetime. Audio runs through
  `processBlock`; UI updates, protocol clients, focus observation and gesture
  timers run on the message thread. Max `qelem`, `defer`, `varname` lookup and
  JS binding teardown are not carried over.
- The plugin deliberately has no `AudioProcessorValueTreeState` or VST3
  automation parameters. Managed remains the committed-state authority; a JUCE
  parameter tree would create a competing store. Host state callbacks capture
  and restore the existing strict Managed snapshot instead.
- The Native DSP graph is compiled directly into the VST3 target and receives
  the host's prepared stereo buffers. Wrapping the Max external or routing
  audio through a second frontend would preserve the wrong host lifecycle and
  add an unnecessary boundary.
- `Features/Equalizer/Presentation/AnalyzerCurveCalculator` is a pure presentation-response
  calculator: it evaluates the accepted filter values at plot frequencies and
  returns display points. It does not process audio or own filter history, so
  moving it into the realtime DSP graph or exporting it through the ABI would
  add the wrong responsibility. Its coefficient equations must remain aligned
  with Native filter behavior; that is a parity concern, not a reason to copy
  the Native runtime cascade and transition state into the UI.
- VST3 has an optional stereo Sidechain input. Both audio pairs pass through
  the existing four-pointer Managed ABI and four-channel capture ring. Managed
  computes main/group/reference FFT and returns one latest fft notification.
  Native device detectors and audio processing remain unchanged.
  See [Vst3SidechainReference.md](Vst3SidechainReference.md).
- JUCE stock window chrome, platform font rasterization and Component
  invalidation replace Max patcher presentation. The UI preserves the
  observable controls and commands, not Max's object graph or pixel renderer.

The shared Managed persistence currently does not capture/restore the Input
detector filter values, although that UI remains editable and its state is
created by Managed defaults. This is a confirmed pre-existing gap affecting
both Max and VST3, not a reason to add a VST3-only state copy. Correcting it
belongs in the Managed persistence contract and must be verified there.

## Client and binding lifecycle

Snapshot routing distinguishes the UI context (`input`) from its parameter
device path (`input_gain`). Analyzer availability is selected by UI context;
its filter projection receives the device path. The 2026-10-14 source review
corrected an availability check against the device path that skipped Input
analyzer snapshots. This correction has not been built or host-tested.
The simultaneous strip uses one graphical editor. Its local selector chooses
EQ or an Input/Saturator/Compressor detector without changing the observed
instance/bank; values come from the already accepted six-context cache.

`ProtocolClient` correlates requests with completions, removes a completion
before invoking it, and reports transport submission failures immediately.
Destroying an editor clears its client completions after final gesture writes
have been submitted and before referenced controls are destroyed.

`TargetStateClient` owns observed instance/bank identity, six pending context
request identities and the combined accepted state bindings. `AnalyzerClient`
owns all-bank request coalescing and strict FFT decoding. All six target
snapshots are decoded and validated into staging storage before publication. Stale replies
cannot replace the current target. The binding layer remains suspended during
transition and resumes after the complete accepted snapshot. Notifications
update accepted bindings without reconstructing all widgets.

History and Input runtime telemetry have separate clients. Runtime values do
not enter committed bindings, write commands or persistence. FFT and all-bank
responses are typed client projections before reaching analyzer presentation.
Registry revisions and topology selection retain the original Max contracts.

Feature controllers translate user intents into existing commands. Controls
own local drag/hover/timer state; bindings own the links between those intents
and accepted client updates. Preview is
separate from committed state and survives until final acknowledgement. One
gesture session serves scalar and analyzer edits. Group floats remain delta
writes; explicit resets and optimization retain Managed history ownership.

The focus session owns the single active viewer, activation generations and
inactive status. Window focus does not become Managed application state.
Inactive overlay blocks the first click from reaching covered controls.

## Implementation sequence and acceptance

1. Organize existing classes into folders and concrete namespaces; update all
   callers and CMake source paths, remove old flat paths.
2. Extract protocol completion, typed target/history/telemetry clients and
   state metadata from the editor. Remove the replaced in-editor implementations.
3. Extract editor focus/session and feature bindings/controllers. Keep editor
   construction, layout and destruction explicit; do not relocate a monolithic
   editor into another monolithic controller.
4. Separate analyzer wire decoding from presentation and rendering. Keep
   topology orchestration outside the bank table widget.
5. Review ownership, message-thread execution, stale responses, binding
   suspension, final gesture writes and documentation. Build and host checks
   require explicit user authorization; DSP automated tests remain prohibited.

Refactoring preserves the existing interface and wire behavior. Acceptance
requires a thin editor, widgets without protocol decoding, clients without
widgets, one production implementation per responsibility, and a documented
Live pass for focus, overlay, local/group gestures, history and save/restore.
Compilation alone does not satisfy host acceptance.

## Progress

Explicit view models are now implemented for state values, parameters, dials,
Input Leveler runtime, analyzer presentation and Tracks selection/rows/
feedback. Bindings own parameter and analyzer models; controls borrow them.
State metadata, formatting, ranges, quantization, curve/spectrum preparation and
action availability have been removed from widgets. Pointer coordinates, hit
testing, drawing and label/flash timers remain in controls.
Input, Saturator, Compressor, Polish and Output are section declarations in
ChannelStrip's `ProcessorCatalog`, including display order, choice labels and
discrete-history paths. Equalizer editing is entirely graphical.
Input bindings attach the module's leveler model to the
shared dial overlay contract. Common parameter models and controls do not
interpret feature paths or Compressor Character labels.

The editor now uses a fixed canvas with a 75/100/125% scale button and simultaneous
processor columns. There is no adaptive grid, parameter viewport or panel
selector. Free window resizing is disabled; the host editor applies the chosen
scale and exact window dimensions. See [Vst3ChannelStrip.md](Vst3ChannelStrip.md) for composition, snapshot
assembly and validation status; the earlier build results below predate this change.

The old Max implementation was then inspected, followed by another pass through
the new frontend. That review found and corrected missing discrete history,
toggle/Leveler reset, track-level revisions and Bank Manager feedback. It also
checked pending history reservations on close and stale discrete callbacks.
The behavior map and concrete reasons for differences are recorded in
[Vst3ParityReview.md](Vst3ParityReview.md); successful compilation is not treated
as proof of full runtime parity.

The plan was recorded before implementation on 2026-10-13. Source stages 1–4
are implemented. `Host/Plugin/PluginEditor` only embeds the presentation, forwards
resize/output and attaches/detaches the host listener. `Host/Editor/EditorPresentation`
composes clients and features, owns local navigation and applies availability;
it does not decode target entries, history, FFT or telemetry or encode state
and topology writes.

Concrete collaborators:

- `Shared/Clients/ProtocolClient`: request/completion ownership and both current error formats;
- `Shared/Clients/TargetStateClient`: target identity, staged snapshots, accepted values and
  ignored notifications during transitions;
- `Shared/Clients/StateCommands`, `Shared/Clients/HistoryClient`,
  `Shared/Clients/InputTelemetryClient`, and
  `Shared/Clients/AnalyzerClient`: their respective existing
  command/projection contracts;
- `Shared/Controls/Parameter/Bindings/ParameterBindings` and
  `Features/Equalizer/Bindings/AnalyzerBindings`: accepted metadata, preview and
  control intents; widgets do not choose group write modes. Bindings own data
  models only; `ChannelStripControl` and `AnalyzerControl` are created by the
  host presentation composition root and borrow those models;
- `Host/Editor/EditorSession`: foreground focus and active-viewer generations;
- `Features/Tracks/Controllers/TrackController` and
  `Shared/Editing/HistoryActionSession`: table commands/topology
  and their existing history ownership;
- `Features/ChannelStrip/Controls/ChannelStripControl`: tile layout;
  `Features/Equalizer/Controls/AnalyzerRenderer`: pure rendering from a
  borrowed presentation view.
- `Shared/Presentation/StateValueViewModel`, `ParameterViewModel`,
  `Shared/Controls/Dial/DialViewModel`, `Features/ChannelStrip/Input/ViewModels/InputLevelerViewModel`, plus the Analyzer and Tracks
  feature view models/feedback: presentation and edit policy, without JUCE
  widgets, command submission or persistence ownership.
- `Shared/Clients/TrackLevelClient`: monotonic runtime revisions and per-instance level cache,
  separate from topology registry state.

Bindings contain no JUCE widget headers or owned Components. Shared
`ParameterBindings` updates accepted parameter models and produces parameter
intents; `ChannelStripControl` creates its `ParameterControl`s and borrows models
from `ChannelStripBindings`. `AnalyzerIntents` is a small typed
contract shared by the Analyzer feature's bindings and control; preview updates
flow through the analyzer model, not through a widget API. The composition
root clears old controls before replacing their borrowed models, and member
destruction order destroys the panel before the model-owning bindings.

Direct source/contract review covered callback removal before destruction,
final writes before viewer deactivation, stale activation replies, transition
suspension and unchanged realtime ownership. No Managed or DSP source changes
were made. Host validation of the refactored frontend remains pending; the
previous UI pass does not validate this refactor.

On 2026-10-14, bindings were further separated from JUCE Components: they now
own/update presentation models and typed intent contracts, while the
`EditorPresentation` composition root creates and replaces controls. Static
source inspection verified that `Shared/Controls/Parameter/Bindings` no longer includes JUCE
headers or widget classes and that the borrowed-model lifetime is ordered.
This structural change has not been compiled or host-tested; the earlier
Release build predates it.

The subsequent component-grouping pass moved every protocol client into
`Shared/Clients`, grouped Dial and Parameter implementation by component under
`Shared/Controls`, and split the host into `Host/Plugin` and `Host/Editor`.
Analyzer transport DTOs moved with `AnalyzerClient` so Shared has no dependency
on Analyzer feature types. The CMake source list now points to the new locations;
the current folder/namespace refactor has not been compiled or host-tested.

Release x64 compilation on 2026-10-13 succeeded with exit code 0 and no warnings
in the build output. The existing configured Managed runtime was reused;
Managed was not built or published. Command from the repository root:

```powershell
& "C:/Program Files/Microsoft Visual Studio/18/Community/Common7/Tools/Launch-VsDevShell.ps1" -Arch amd64 -HostArch amd64 -SkipAutomaticLocation
Remove-Item Env:CMAKE_GENERATOR_PLATFORM -ErrorAction SilentlyContinue
cmake --build Consolidator.Vst3/build --config Release --target Consolidator_VST3 --parallel 4
```

The process-local inherited CMake platform environment variable was removed
before the build to avoid the previous CMake environment warning. This does
not modify persistent system settings. The built bundle was copied to
`C:/Program Files/Common Files/VST3/Consolidator.vst3`; the source and installed
module SHA-256 hashes match. No unit/integration tests or Live UI automation
were run for this refactor.

After explicit view-model extraction and the parity corrections, the same
Release x64 command was run again on 2026-10-13. Final build exited with code
0 and no warnings in its output. The installed bundle was updated again; the
built and installed module hashes match. The existing published runtime in
`Consolidator.Vst3/build/managed-runtime` was reused, with no Managed build or
publish. This verifies compilation/installation, not the current Live behavior.
