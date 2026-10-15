# Max UI to VST3 source review

## Scope and evidence

On 2026-10-13 the existing Max frontend was inspected first, then the VST3
clients, bindings, view models, controls, feature controllers and host wiring
were inspected again. This is a source/contract review, not proof of runtime
parity. Managed and Native DSP implementations were not changed. No automated
DSP tests, audio measurements or listening were performed.

The old Max-side pass covered the Max `Shared/Clients`, `Shared/Bindings`,
`Shared/ViewModels`, Dial/Button/MultiValueToggle presentation and interaction,
all six feature view models/controllers, analyzer scale/editor/curve/rendering
and gesture transport, and Bank Manager selection/actions/topology/feedback.
The current Managed protocol encoders were read to resolve wire-contract
questions without modifying them.

## Behavior map

VST3 paths below are relative to `Consolidator.Vst3/Source`.

| Old responsibility | Current implementation / review outcome |
| --- | --- |
| `NativeProtocolClient` requests, responses and disposal | `Shared/Clients/ProtocolClient`, host transport; callbacks removed before invocation and cleared before referenced objects are destroyed; current four/five-atom error bodies supported |
| Target generation, full snapshot and subscription readiness | `Shared/Clients/TargetStateClient`; request/target identity checked, complete entries staged, transition notifications ignored, bindings disabled until accepted batch |
| `StateValueViewModel` values, ranges, enabled state and unchanged-entry suppression | `Shared/Presentation/StateValueViewModel`, `ParameterViewModel`; accepted metadata, explicit enablement and equality suppression are outside widgets |
| Per-context parameter order and Compressor Character choices | `Features/ChannelStrip/Presentation/ProcessorCatalog.h`; section declarations contain ordering, choices and exact history-path metadata |
| Discrete edit history for Input/Saturator/Compressor/Output | Section `historyPaths` retain leveler, split, Character/parallel and limiter policy; host composition handles the unchanged shared transaction lifecycle |
| Input Level/Target/Leveler/Width | Macro order retained; `Features/ChannelStrip/Input/ViewModels/InputLevelerViewModel` owns runtime display/read-only state and `Features/ChannelStrip/Input/Bindings/InputBindings` connects it to accepted state and telemetry; `Shared/Clients/StateCommands` retains dedicated grouped disable command |
| Saturator Drive/Curve/Split/Output | All four controls and detector retained |
| Compressor Attack/Sustain/Compression/Character/Parallel/Output | All six controls and detector retained; Character labels, three points and vertical sensitivity preserved; pointer hover no longer substitutes a choice for the accepted value |
| Polish Thick/Air; Output Level/Target/Limiter | All controls retained, including dB/percent presentation |
| EQ seven filters in all seven banks; two detector filters | `AnalyzerViewModel` uses the existing type/fixed-Q catalog and accepted parameter metadata; no frontend DSP defaults are applied |
| Dial ranges, snapping, unsnapped accumulation, preview and labels | `Shared/Controls/Dial/DialViewModel`; Shift enables snapping only during that dial's drag; widgets retain pointer coordinates and 500 ms label timer; group interval uses physical-scale geometry |
| Toggle/Character history and toggle reset | Actual `UiBindingInstaller` supplies TransactionClient; macro values now begin/write/end with that ID, toggle double-click resets, Leveler reset uses dedicated disable |
| Input second relative ring | Slow and fast runtime pair separate from committed state; -6/+6 dB inner ring visible only while Leveler is enabled |
| Analyzer position/Q editing and reset | `Features/Equalizer/ViewModels/AnalyzerViewModel` prepares continuous values; `Features/Equalizer/Bindings/AnalyzerBindings` chooses copy/delta; pointer widget emits typed edits; linear Q accumulation and effective-range checks retained; global precision modes, snapping and Rough dots intentionally removed |
| Local response, all-bank overlay and FFT | `Features/Equalizer/ViewModels/AnalyzerViewModel`, `Shared/Clients/AnalyzerClient`, `Features/Equalizer/Controls/AnalyzerRenderer`; fixed Q, summed bank overlay and 0.5/0.16 FFT smoothing retained; bypassed filter shapes remain visible in gray and contribute nothing to the sum; intentional denser response grid, peak-preserving spectrum columns, synchronized overlay preview and unclamped paths clipped by the plot are documented in [Vst3AnalyzerRendering.md](Vst3AnalyzerRendering.md) |
| Registry snapshots/deltas and markers | `Shared/Clients/RegistryClient`; revision gaps request replacement, seven distinct banks required, marker updates do not fetch topology |
| Track-level revision and per-instance cache | Separate `Shared/Clients/TrackLevelClient`; stale revisions ignored, full frame validated before mutation, levels do not advance registry revision |
| Bank selection, group focus, action availability, labels and level alpha | `Features/Tracks/ViewModels/TracksViewModel`; one selected bank per instance, Shift toggling/visited drag, automatic group 0 and creation limit 1..15 retained |
| Bank/processor bypass preview and action flashes | `Features/Tracks/ViewModels/TrackFeedback` owns temporary bypass overrides; confirmed presentation clears matching overrides; `Shared/Controls/ActionButton` preserves 180 ms row-reset/toolbar/history feedback |
| Mute/Solo/Bypass and targeted resets | `Features/Tracks/Controllers/TrackController`; additive/exclusive and focused-bank scope rules preserved; instance Reset still addresses current target; no extra reset history point |
| Group/Ungroup/Clear/Commit/Refine history | Existing commands preserved; topology writes queued before history end; optimization/reset history stays in Managed |
| Callback, timer and model disposal | Host removes output listener first; presentation submits final gesture writes, deactivates viewer, clears completions, detaches viewport; controls die before referenced models; timers stop during destruction |

## Corrections found during the two passes

- The first VST3 implementation kept derived dial/analyzer/table presentation
  inside widgets. Explicit view models now own those responsibilities; the
  replaced fields and preparation methods were removed from controls.
- Track levels previously lacked the original monotonic revision check and
  could partially apply a malformed frame. The separate client stages all
  values before updating its cache.
- Normal focus clears the previous explicit selection, including when the new
  bank is grouped. `BankManagerSelection.selectBank` first invokes
  `setFocusedBank`, which clears selection even for the already focused bank;
  only then can `toggleBankSelection` select an ungrouped bank. The earlier
  review incorrectly considered `canSelectBank` without the preceding focus
  step. The VST3 now clears first, then selects only an ungrouped bank. Shift
  toggling remains additive and grouped banks remain ineligible for it.
  Processor navigation to another instance and deletion of the focused
  instance also clear the old explicit selection.
- Bypass overrides and the 180 ms reset/action flashes were absent. They have
  been restored as transient presentation, with no speculative rollback of
  confirmed Managed state.
- The `state_changed` status slot was incorrectly described as a revision in
  C++. It is the literal `ready`; decoding now checks that current contract.
- Analyzer frequency grid rounding now matches the old nearest-pixel rule.
- Q editing retains the old out-of-effective-range epsilon check rather than
  silently clamping an obsolete raw gesture position into a new range.
- The earlier assumption that all discrete controls write without frontend
  history was incorrect. The actual installer, rather than a binding's optional
  constructor branch, determines this behavior. Macro toggles and Character
  now preserve it; table bypass/mute/solo remain direct commands.
- Toggle double-click reset was missing, and generic Leveler reset would lose
  the slow-gain disable behavior. Both paths were corrected.
- Pending discrete edits now validate target generation/scope/session after
  begin acknowledgement. An old UI callback cannot write through a replacement
  target. Closing also ends history reservations still awaiting begin.

## Differences with a concrete reason

- The six old feature view-model containers mostly construct named
  `StateValueViewModel` objects and dispose them. C++ uses an owned model per
  accepted path, the same macro ordering and shared `ParameterViewModel`
  composition. Input runtime, analyzer and Bank Manager retain their own
  specialized models. Six duplicate property-wrapper classes add no behavior.
- Max selector/atom presentation encoding, varname lookup and JS subscription
  teardown are replaced by typed C++ updates and RAII. Complete snapshot
  suspension and destruction ordering remain explicit; wire commands still
  cross the unchanged Managed protocol.
- A VST3 editor can be destroyed while its processor remains alive. An action
  still waiting for history begin is canceled on close, and its point is ended.
  Already submitted writes still finish. This prevents orphaned reservations
  and callbacks into a destroyed view.
- The old generic dial supports up to three rings. Its actual feature usage is
  one main ring plus Input's fast relative ring; both are retained. Unused
  generic ring configuration and visualization scaffolding is not copied.
- JUCE arcs measure angle from the top, while Cairo/mgraphics measures from
  the right. The numeric start angle differs by pi/2; the rendered 240-degree
  sweep and Character point geometry follow the original coordinates.
- The original analyzer scale reads effective limits even in local scope.
  VST3 uses physical limits for local writes and effective limits for group
  delta writes, matching the explicit scope contract and the original dial
  policy. This fixes an old presentation restriction rather than removing
  group constraints.
- The old response calculator can use a synthetic 48 kHz default. VST3 uses
  the prepared host rate and stays blank before audio preparation; otherwise
  response presentation could disagree with the actual Native graph rate.
- Window focus and the Inactive overlay replace manual activation as explicitly
  requested. Max selected-track/patcher observation has no corresponding VST3
  host API here; host labels and explicit Bank Manager focus are used.
- The earlier native responsive panel exposed numeric filter parameters as
  an additional precise-edit surface. The fixed channel strip now removes
  these duplicate controls as explicitly requested; filters are edited in
  the graphical analyzers, including numbered bypass/reset buttons. See
  [Vst3ChannelStrip.md](Vst3ChannelStrip.md). Font rasterization and stock host
  window chrome are platform differences, not evidence of identical screenshots.

## New-code pass and remaining validation

The second pass checked model/control ownership, staged snapshots, callback
correlation, stale activation generations, range/precision policy, preview
lifetime, grouped Leveler disable, registry versus level revisions, explicit
history ownership and the unchanged audio-thread boundary. No confirmed
functional omission found in this scoped pass remains intentionally unfixed.
This does not guarantee that every runtime edge or pixel is identical.

The final persistence inspection found a pre-existing shared Managed omission:
the original Input UI exposes editable detector filters, and
`ManagedStateBuilder.CreateInput` creates their state, but
`InstancePersistenceService.CreateInput` and `ApplyInput` do not capture or
restore them. Saturator and Compressor detectors are explicitly persisted.
The VST3 uses this same Managed snapshot, so this omission also affects it;
it is not a removed ViewModel feature. Managed remains unchanged as requested.
The missing capture/restore is confirmed by source inspection; a project-reload
reproduction has not been performed.

A new Live pass is still required for focused/inactive windows, editor reopen,
rapid gestures, grouped edits, history, project reload and audible operation.
The earlier host pass does not verify these current view models and feedback
changes. Build/install results are recorded in
[Vst3Architecture.md](Vst3Architecture.md).

The final view-model/history/reset/feedback/revision implementation compiled
successfully in Release x64 with no warnings in the output. The installed
bundle was updated and its module hash matched the artifact. No new Live pass
or unit/integration test execution was performed for these corrections.
These build results predate the simultaneous fixed channel-strip layout and
six-context snapshot assembly, which have undergone source review only.

On 2026-10-14, the grouped-bank selection correction was compiled in Release
x64 and installed successfully with exit code 0 and no warnings. The installed
module hash matched the artifact. Direct comparison followed both
`BankManagerSelection.selectBank` and `TracksViewModel.setFocusedBank`,
including repeated focus, Shift extension and processor navigation. No new
Live interaction pass or automated test execution was performed for this fix.
