# VST3 feature boundaries

## Architecture

The UI has three feature scopes, independent of where a control is drawn:

| Scope | Responsibility |
| --- | --- |
| `Features/Tracks` | Desired UI track/bank/scope, selection, groups, registry/level presentation, track list, shared bank-cell behavior and S/M/R/B controls |
| `Features/ChannelStrip` | One five-column processor layout, section metadata, discrete-history declarations, parameter binding composition and Input Leveler presentation |
| `Features/Equalizer` | Complete EQ editor: graph, source selector, actions and vertical banks, response/FFT caches |
| `Host/Editor` | Feature composition, protocol event routing, window focus, activation, sizing, sidebar actions and layout application |
| `Shared` | Protocol clients, generic accepted-value controls, gesture/history mechanism, theme and runtime overlay contracts |

Tracks controls can appear in the table, processing title and analyzer margin;
they share models and behavior rather than belonging to their screen location.
The former Bank Manager is named Track List in frontend code/UI. Banks and groups
remain existing Managed concepts; no wire names or ABI structures are renamed.

Processor sections are metadata within ChannelStrip, not separate top-level
features. Input retains its own leveler binding/model module because its telemetry
and edit restrictions are behavior, not duplicated layout. Discrete history
paths live in section declarations instead of one-method controller classes.
Generic parameter controls/bindings remain independent of feature headers.

Tracks owns the single desired UI target and scope. TargetStateClient retains
request/accepted-context correlation, generations and transition readiness;
these protocol responsibilities are not merged with selection. Shared history,
application state, propagation, optimization and persistence remain Managed-owned.

EditorLayout computes rectangles for the expanded/collapsed fixed canvases.
Feature controls apply their own internal layout; the host places them and
connects intents. There are no compatibility aliases or old feature facades.

## Completed source changes

1. Renamed source/namespaces/classes to Tracks/Track List and updated CMake/callers.
2. Consolidated processor descriptors/history paths in ProcessorCatalog and retained Input behavior.
3. Moved the fixed view to ChannelStripControl; ChannelStripBindings owns Input/parameter binding composition.
4. Removed editor target/scope fields and TrackList forwarding APIs; TracksViewModel owns desired selection context.
5. Extracted pure EditorLayout rectangles and updated the architecture/current VST3 references.
6. Reviewed source ownership, destruction order, target generations, group/command flows and retained UI actions.

The pending UI changes (continuous analyzer/Q, bypassed response shapes, Shift-only
dial snapping, larger track indicators, bank column and shared track action strips)
must be preserved. Managed, Native DSP and the protocol are outside this refactor.

## Verification

The user authorized the Release x64 build and installation on 2026-10-14.
After restoring the accidentally deleted Tracks sources and regenerating CMake,
ConsolidatorVST3Install completed with exit code 0 and no compiler/linker warnings.
The installed module SHA-256 matches the artifact:
`4CB15F0C95FDCBB2D65F01CB3950D8D2A585C523E28D2FE219E915B2F9FE94EB`.
Existing Managed runtime was reused without rebuilding or publishing Managed.
No tests or Git operations were run.

Source review on 2026-10-14 covered the old five profile orders, Character labels
and exact discrete-history paths; leveler overlay lifetime; control teardown
before replacing borrowed models; target deletion and activation/rejection;
group focus, selection, S/M/R/B, bank/processor bypass/reset, Undo/Redo,
Commit/Refine and fixed geometry at either visibility state. Previously requested
pending controls/gesture/rendering changes remain in the same source tree.
Compilation and host behavior are unverified; the source review is not a build
or test result.

## Equalizer section consolidation

On 2026-10-14 the analyzer feature became Equalizer. EqualizerControl owns the
analyzer, EQ Bypass, Commit/Refine, source selector and vertical BankColumnControl.
Its subsequent fixed internal layout is documented in Vst3ChannelStrip.md; EditorLayout
now places one 1060 x 338 section at the former action-panel origin.
Analyzer bindings, model, renderer and curve calculation use the equalizer
namespace/folder. BankColumnControl moved from Tracks into Equalizer; the table
and column still share TracksViewModel, BankCellRenderer and BankCellInteraction.

The host retains protocol routing, target generation guards and command/history
coordination. It no longer owns the EQ buttons/source selection or lays out the
section's children. The section controls accepted bypass state, group scope/color
and navigation/gesture enablement. Graph() exposes the actual owned graph for
stream updates instead of adding duplicate forwarding APIs.

Source review covered shared selection/feedback, group scope, Ctrl bypass/reset,
source changes, accepted-value updates, gesture locking, fixed geometry and
bindings/control destruction order. This subsequent change has not been built,
installed or tested. Managed, Native DSP and protocol contracts are unchanged.