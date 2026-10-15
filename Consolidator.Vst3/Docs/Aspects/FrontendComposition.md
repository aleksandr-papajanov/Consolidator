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
| `Features/ChannelStrip` / `features::channelstrip` | Gain Staging double column plus three processor columns, binding composition and presentation metadata |
| `Features/ChannelStrip/GainStaging` / `features::channelstrip::gainstaging` | Gain Staging input-leveler runtime overlay and edit restrictions |
| `Features/Equalizer` / `features::equalizer` | Complete EQ section: analyzer, source selector, EQ Bypass, Commit/Refine, vertical banks, response/FFT presentation and graph gestures |
| `Shared/Clients` / `shared::clients` | Typed protocol clients and accepted projections, with no widget/feature dependencies |
| `Shared/Protocol` / `shared::protocol` | Owned frames, strict wire decoding and command value types |
| `Shared/Editing` / `shared::editing` | Generic gesture intent transport and parameter intents |
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
ChannelStripBindings composes generic ParameterBindings with the Gain Staging module.
ProcessorCatalog declares four sections, parameter order and Character choice
labels. It contains no history policy. Managed state definitions and commands
own history participation, group propagation and dependent edits. Generic
controls/models do not duplicate those decisions per processor.
Shared remains organized by reusable component. Generic bindings own value models,
not Component instances. Controls emit typed intents rather than commands.
Clients never include widget/editor/feature headers. Protocol cannot depend on
host, clients or presentation. EditorLayout consumes only visibility/text metrics
and computes fixed rectangles; it does not inspect state or issue commands.
EditorPresentation applies geometry and connects existing clients, feature
bindings, views and controller actions. Managed remains the committed-state owner.

## JUCE lifecycle and state boundary

- `AudioProcessor` owns per-plugin audio/runtime/Managed registration; the
  `AudioProcessorEditor` and its child `Component`s are disposable views.
  Presentation clients and controls therefore live with `EditorPresentation`,
  not the processor.
- JUCE owns host audio callbacks and component lifetime. Audio runs through
  `processBlock`; UI updates, protocol clients, focus observation and gesture
  timers run on the message thread.
- The plugin deliberately has no `AudioProcessorValueTreeState` or VST3
  automation parameters. Managed remains the committed-state authority; a JUCE
  parameter tree would create a competing store. Host state callbacks capture
  and restore the existing strict Managed snapshot instead.
- The Native DSP graph is compiled directly into the VST3 target and receives
  the host's prepared stereo buffers. No second frontend participates in the audio path.
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
  See [Vst3SidechainReference.md](SidechainReference.md).
- JUCE owns window chrome, font rasterization and Component invalidation.

Managed persistence schema 6 includes the retained Gain Staging detector; see
[GainStaging.md](../../../Consolidator.Managed/Docs/Aspects/GainStaging.md).

## Client and binding lifecycle

Gain Staging uses the same `gain_staging` processor ID and parameter prefix.
The simultaneous strip requests Gain Staging, Saturator, Compressor, Polish and
Equalizer snapshots for one target/bank. The main graph's source selector uses
these accepted values without additional target ownership or subscriptions.

`ProtocolClient` correlates requests with completions, removes a completion
before invoking it, and reports transport submission failures immediately.
Destroying an editor clears its client completions after final gesture writes
have been submitted and before referenced controls are destroyed.

`TargetStateClient` owns observed instance/bank identity, five pending context
request identities and the combined accepted state bindings. `AnalyzerClient`
owns all-bank request coalescing and strict FFT decoding. All five target
snapshots are decoded and validated into staging storage before publication. Stale replies
cannot replace the current target. The binding layer remains suspended during
transition and resumes after the complete accepted snapshot. Notifications
update accepted bindings without reconstructing all widgets.

History and Input runtime telemetry have separate clients. Runtime values do
not enter committed bindings, write commands or persistence. FFT and all-bank
responses are typed client projections before reaching analyzer presentation.
Registry revisions and topology selection follow the Managed protocol contract.

Feature controllers translate user intents into existing commands. Controls
own local drag/hover/timer state; bindings own the links between those intents
and accepted client updates. Preview is
separate from committed state and survives until final acknowledgement. One
gesture session serves scalar and analyzer edits. Group floats remain delta
writes; explicit resets and optimization retain Managed history ownership.

The focus session owns the single active viewer, activation generations and
inactive status. Window focus does not become Managed application state.
Inactive overlay blocks the first click from reaching covered controls.

## Review boundary

Read the [project rules](../Rules.md) before changing a control. Trace an
intent through Managed validation, state/peers/history/dependent edits and DSP
publication, then through accepted snapshots/notifications to the bound model.
The frontend may preview a gesture; it must not commit application state or
choose which parameter requires history. Tracks sends group/ungroup intents;
Managed allocates group IDs and applies membership in one transaction.

Controls are destroyed before their borrowed models. No editor callback may
outlive its clients/session. Native audio processing is independent of editor
presence. Source review, build and Live validation are separate; earlier build
logs do not prove current behavior. See [Build](Build.md) and [Validation](Validation.md).