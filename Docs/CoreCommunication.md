# Core communication architecture

`ConsolidatorCore` owns one process-wide `InstanceCoordinator`, registry and
coordinator worker. Instances register their topology and coordinator-owned
`StateStore` during construction.

## Command flow

```text
Max / control code
  -> ConsolidatorInstance::EnqueueCommand()
  -> InstanceCoordinator command queue
  -> CommandRouter resolves topology and group targets
  -> StateWriter returns response + effects
  -> InstanceCoordinator publishes response and applies effects
  -> per-instance RuntimeUpdateMailbox (latest-value slots)
  -> audio thread: ConsumeLatest()
  -> DspChain::ApplyRuntimeUpdates()
  -> DspChain::Process()
```

`StateStore` is the only authoritative owner of user-facing parameters,
physical ranges, banks, groups and selected bank. Its topology component is
`InstanceState`; DSP parameter state is stored in `ChainState`. Both are
accessed through `StateStore` and are written only by the coordinator worker.
Reads are also resolved from `StateStore`; the coordinator never reads a live
DSP chain.

`ConsolidatorInstance::Initialize()` registers the fully constructed instance,
then registers every DSP `StatePath` and publishes the complete initial runtime
state before returning to the external owner. `Process()` is not exposed by the
external wrapper before this initialization completes.

`RuntimeUpdateMailbox` has one producer (the coordinator worker) and one consumer
(the instance audio callback). During instance initialization it registers a
fixed slot for every DSP `StatePath`. Publishing replaces the value in that
path's slot and never requires a retry queue; an unregistered path is a fatal
invariant violation. Parameter and processing updates each carry their own
monotonic revision sequence for diagnostics and ordering within their mailbox.

`RegisterPath()` completes before the first `Process()` call. The mailbox is
intentionally single-producer: concurrent publishers are not supported by the
seqlock protocol.

Non-coalescable actions such as reset must use a separate event queue.

The two audio-thread delivery mechanisms have different semantics:

- `RuntimeUpdateMailbox` carries persistent runtime state where the latest value
  wins;
- `SpscQueue<RealtimeCommand>` carries ordered events where every command must
  be observed.

`Bypass` and `Solo` are authoritative processing markers carried by
`WriteStateCommand`, so they use the same validation and grouping flow as other
state parameters. Neither marker is forwarded to the ordinary DSP mailbox.
After the state commit, `ProcessingStateResolver` enqueues internal
`RuntimeControlUpdate` values through a shared runtime-control mailbox. Each
update carries a `RuntimeProperty` (`Active`, `Listen`, or `OutputEnabled`), and the mailbox key
includes both target path and property. They are
derived runtime values and are not state-protocol entries. At the start of an
audio block the parameter and processing snapshots form one runtime boundary:
parameters are committed first, then routing flags are applied, then audio is
processed. Processing updates only change active flags and do not require
`CommitRuntimeUpdates()`.
At the start of a block the instance applies parameter updates, then processing
updates, then ordered reset events, and only then calls `Process()`. Reset events
therefore observe the newly committed runtime configuration.

`ResetDspCommand` is delivered through the instance's fixed-capacity SPSC event
realtime command queue and resets the selected device route's real-time memory on
the next audio block;
it does not change user-facing parameter values.
Reset routing follows the same route hierarchy as DSP parameters, including EQ
banks/filters and detector filters; each composite device consumes one route
segment and delegates the remainder to its child.

## State protocol

Addressable state uses two self-contained command types: `ReadStateCommand`
with bounded `queries` and `WriteStateCommand` with bounded `entries`.
`Command` is their variant. The `ConsolidatorInstance` fills `instanceId`
before enqueueing. `StatePath` is a prefix query for reads and a complete address
for writes. State requests produce one consolidated `StateResponse` addressed
by the same `instanceId`; it contains `requestId`, `entries`, and `truncated`.
Action requests produce an `ActionResponse` with `requestId`, `instanceId`, and
`ActionStatus`.
The Max-side atom contract, wire correlation and batch limits are fixed in
`Docs/MaxProtocol.md`; Core does not expose the wire `source`.
`StateStore` owns both topology
entries and all DSP parameter entries.

Valid path factories include:

```cpp
StatePath::Instance(instanceId);
StatePath::InstanceMute(instanceId);
StatePath::InstanceSolo(instanceId);
StatePath::SelectedBank(instanceId);
StatePath::BankGroup(instanceId, bankId);
StatePath::DspParameter(instanceId, route);
```

DSP parameters use `StateField::DspParameter` and `ParameterId`. DSP markers
use `StateField::DspMarker` and `StateMarkerId`; markers are not parameters.

`ParameterConstraintResolver` reads only `StateStore`. Write routing and
constraint dependencies are intentionally separate:

- `GroupGraph` owns topology traversal through:
  `GetGroupMembers()`, `GetGroupedBanks()`, and `GetConnectedGroupBanks()`.
- `StateRouter::ResolveSourceBank()` selects the path's bank or the source
  instance's selected bank.
- `StateRouter::ResolveWriteTargets()` returns one direct group, falling back to
  the source bank when it is ungrouped, and collapses instance-owned targets.
- `StateRouter::ResolveConstraintTargets()` returns the direct group for
  bank-scoped paths and the connected topology component for instance-owned
  parameters.
- `StateRouter::Retarget()` adapts a path to a target instance and bank;
  `IsBankScoped()` identifies the current bank-owned EQ paths.

Constraint traversal walks connected group components transitively, so a chain
such as A → B → C is handled correctly. Group-linked writes use only the direct
group and are validated for every write target before any target is mutated,
then committed to each target store and returned in one coordinator response.
Constraint enrichment runs after the complete plan is committed.

After a parameter commit, the response also includes authoritative values for
every target returned by `ResolveConstraintTargets()`. This keeps
effective limits synchronized for other grouped instances, including writes
that remain local because the selected bank is not grouped.

One coordinator write plan publishes all runtime updates for an instance before
the response is emitted. The audio thread scans the fixed slots once at the
start of its block and stages the resulting local batch; values superseded
before that scan are collapsed by path.

The coordinator routes each command response to the `ConsolidatorInstance` whose
`instanceId` is carried by the response. Every instance owns its own response
queue of `StateResponse` and `ActionResponse` values, and its owner reads
responses through `TryDequeueResponse()`; there is no process-wide response
queue. For `ResetDspCommand`, `ActionStatus::Accepted` means that the reset
event was placed in the instance realtime queue; it does not mean that the DSP
has already executed it. `Applied` means that the authoritative
`StateStore` changed and the instance mailbox accepted the runtime update; it
does not mean that the audio thread has already applied it. There is no local
audio command/response queue for state access and no `ParameterStateView`.
`ConsolidatorInstance::SetResponseNotifier()` signals the external transport
after enqueueing a response; the callback may make only the thread-safe Max
scheduling call (`queue<>.set()`). It must not call outlets, emit messages, or
encode atoms from the coordinator thread. It must be installed before
`Initialize()` and cannot be changed afterward. Instance destruction first
deactivates the notifier and then unregisters the instance, so a queued worker
notification cannot call into a destroyed external. The notifier mutex is held
through callback completion; destruction therefore waits for an in-progress
wake-up before the external-owned callback target may be destroyed.

## Core component boundaries

`Core/Domain/State` contains the authoritative state model and `StateStore`.
`Core/Routing` contains command routing, group traversal, constraint resolution
and response delivery. `InstanceCoordinator` remains the composition root and
owns the coordinator worker, command queue and routing components, but routing
does not belong to the coordinator lifecycle itself.

The state implementation is split by operation without introducing additional
state-owner objects:

```text
Core/Domain/State/
├─ StateStore.h
├─ StateStore.cpp
├─ StateStore.Factory.cpp
├─ StateStore.Read.cpp
├─ StateStore.Write.cpp
├─ StatePath.h
├─ StateEntry.h
├─ ParameterState.h
├─ StateMarker.h
├─ InstanceAudibilityState.h
├─ InstanceState.h
├─ ChainState.h
└─ DspStates.h
```

`StateStore.cpp` owns construction, `StateStore.Factory.cpp` builds a complete
`ChainState` from `DspSettings`, and the read/write translation remains in the
two operation-specific implementation files. Command transport types live in
`Core/Domain/Commands/StateProtocolCommands.h`; they are separate from the
state address and entry representation.

## DSP boundary

DSP devices do not own authoritative user state. They receive runtime updates
from `RuntimeUpdateMailbox`, update local `RuntimeState`, and process audio. A
runtime state may contain a local copy of target values, coefficients,
smoothing values and audio memory, but it is never read by the coordinator as
the source of truth.

Topology changes include affected-bank parameter entries in the response so
the UI receives refreshed effective ranges after group changes.

## Global analysis service

`AnalysisService` is a process-wide service independent of
`InstanceCoordinator`. It starts one background analysis worker and owns one
fixed analysis slot per registered `ConsolidatorInstance`.

The audio thread of the instance in the current view accumulates stereo FFT
windows for its main and reference inputs and publishes completed windows into
the slot's latest-input states. The worker reads the newest windows, calculates
the spectra, and publishes persistent latest results. FFT windows and analysis
jobs are never kept in a FIFO: a newer window replaces an older pending one.
The worker uses adaptive polling: it yields after a productive pass and sleeps
for 2 ms when no stream has work. The audio thread performs no wake-up,
mutex, or scheduler operation.
The worker acquires the current view and its `shared_ptr` slot under the
registry mutex on each polling cycle, then processes that slot without holding
the mutex. Non-viewed slots do not accumulate or process spectra.
Service destruction explicitly requests worker stop and joins it before the
service-owned analysis state is destroyed.
Spectrum result revisions belong to consumers, not slots. Result reads are
persistent, so multiple physical externals can read the same output and keep
their own last-seen revision. Audio-window input remains a one-consumer
destructive stream.
`SpectrumStream` encapsulates accumulation, input publication, worker
consumption and output publication; `AnalysisService` does not access its
internal buffers or revisions.
Spectrum-specific types and the stream now live under `Analysis/Spectrum`,
separate from the service and buffer primitives.
The independent theoretical response calculator lives under
`Analysis/FrequencyResponse`. Core converts authoritative `ChainState` into
the analysis-specific immutable `CurveInput`; Analysis converts that input
into one normalized request for the bank in the current view and produces 256
logarithmically spaced magnitude dB points from 20 Hz to
`min(20 kHz, Nyquist)`.
Both DSP filters and analysis use the shared pure `dsp::BiquadDesigner`, so
Bell, shelf, gain, and Tilt component coefficients have one implementation.
The standard seven-band topology is defined once in
`Dsp/Processors/Equalizer/EqualizerLayout.h` and is shared by DSP
construction and analysis request building.
The curve result contains one snapshot per EQ filter, a combined snapshot for
the current bank, and an `allBanksCombined` snapshot containing the aggregate
curve for every active bank. A Tilt filter's individual snapshot contains its
low- and high-shelf stages. `AnalysisService::TryReadLatestCurve(snapshot,
lastRevision)` exposes this aggregate result to the UI. Every result has a
service-wide result revision and a view epoch;
the source revision remains available inside each curve snapshot.
`ConsolidatorInstance::Prepare(sampleRate)` forwards the sample rate to its
analysis slot and atomically marks the curve input stale. The coordinator
worker then republishes immutable curve input from its authoritative
`StateStore`; `Prepare()` never reads coordinator-owned state. Each display
`SpectrumSnapshot` carries the rate captured with its source window alongside
its magnitudes and revision.
The analysis service exposes the current-instance main spectrum, reference
spectrum, their dB difference, and the EQ curve bundle through persistent
latest-result readers. Readers pass their own last-seen result revision, so
reading never consumes or removes a snapshot. `ConsolidatorExternal` does not
own an analysis worker and does not receive analysis notifications. Its
`analysis_view <instanceId> <bank>` selects the global analysis view. The
argument-free `analysis_tick` message is driven by the Max/UI refresh loop and,
on the Max main thread, reads every latest result
whose revision changed and emits the changed frames through one
`analysisOutput` outlet. The selectors are
`spectrum_main`, `spectrum_reference`, `spectrum_difference`, `eq_filter` with
a 1-based filter id, `eq_combined`, and `eq_all_banks`. This coalesces worker
updates to the UI refresh rate while keeping analysis independent of Max.
The difference is calculated as `main - reference` after both spectra have
been produced for the current view epoch. `TryReadLatestSpectrum(snapshot,
lastRevision)` and `TryReadLatestReferenceSpectrum(snapshot, lastRevision)`
expose the source snapshots independently;
`TryReadLatestDifferenceSpectrum(snapshot, lastRevision)` exposes the derived snapshot without
requiring the consumer to synchronize the two source cursors.
The rate is captured into each immutable `AudioWindow` by the audio-side
accumulator; `SpectrumStream` has no mutable sample-rate state shared
with the worker.
Window revision travels with `AudioWindow`; the stream does not carry a
separate input revision variable through the processing call.
`AnalysisService` owns one `SpectrumAnalyzer` and one reusable KissFFT
configuration. All slots share it because the single analysis worker performs
FFT calculations serially.
The analyzer implementation lives under `Source/Analysis/Spectrum`; the
service only schedules slots and publishes their completed results.
`SpectrumAnalyzer` precomputes its Hann window once and reuses preallocated FFT
input/output buffers for every calculation.
Raw magnitudes use separate left/right FFTs, Hann coherent-gain normalization,
and one-sided amplitude normalization: DC and Nyquist are not doubled, while
interior bins are doubled. The channels are combined as spectral power,
`sqrt((|L|^2 + |R|^2) / 2)`, so opposite-phase stereo signals do not cancel.
The latest window revision is sufficient to identify pending analysis work;
the worker keeps its own processed revision and skips a window when both
revisions match. The large preallocated sample buffer is therefore not routed
through a second mailbox.
Audio windows use the destructive `LatestSnapshot<T>` transport with
preallocated ownership-state buffers: the worker consumes a published window
and releases its storage for reuse. Worker results use the persistent
`LatestValue<T>` transport instead. `ReadLatest()` can return the same result
repeatedly, while `TryReadNewerThan()` lets each UI consumer keep its own
revision cursor. This allows multiple physical externals to reopen the same
instance/bank without requiring another state or audio change.
The FFT transform itself uses the vendored BSD-3-Clause KissFFT sources under
`Source/Analysis/KissFFT`.

The slot handle is registered after the instance receives its `InstanceId`.
During destruction the instance first leaves the coordinator registry; only
then is the analysis slot unregistered and released. Analysis does not
participate in command routing or authoritative state ownership.

## Lifecycle

### Analysis view

`AnalysisService` is global and owns one latest-value `AnalysisSlot` per
registered instance plus one worker. The UI selects an
`AnalysisView { instanceId, bankId }`; this view is independent from the
instance's `selectedBankId`. Instances publish only latest audio windows and
immutable curve input state. Curve state is persistent latest-value data so a
bank switch can recalculate from the same state without waiting for another
state change. The worker calculates FFT for the instance in the current view
and calculates the selected-bank curves plus the all-banks aggregate, while
the UI reads all snapshots from
`AnalysisService`. Worker processing revisions and UI consumption revisions
are separate. Curve processing state is owned by each curve stream rather
than by the global service, so additional result streams do not require more
service-wide processed-revision fields. The registry mutex is held only while
acquiring the current `shared_ptr` slot and view metadata; FFT and curve
calculation run without
the registry lock. Each slot has an atomic spectrum-enabled flag; only the
current view's slot accumulates and publishes audio windows on the audio
thread. Switching a slot from disabled to enabled requests an audio-thread
accumulator reset. `SpectrumStream` owns the atomic window generation at the
audio/worker boundary; the accumulator itself is audio-thread-only. A partial
FFT window therefore cannot combine audio from two separate view intervals,
and the worker never reads accumulator internals.
Published spectrum and curve snapshots also carry the view epoch. The service
rejects snapshots from an older epoch, preventing a cached result from a
previous visit to the same instance from being returned after a view switch.
Successful UI reads return the matching `AnalysisView` under the same registry
lock as snapshot validation, so a concurrent view switch cannot attach metadata
from another view to the result.
Every published result also receives a service-wide monotonic result revision;
it changes for new audio, state, instance, or bank results independently of
the source input revision. `GetView()` returns `std::nullopt` when no view is
selected; unregistering the viewed instance clears the view instead of using a
sentinel instance ID.

Instance unregistering is serialized by the coordinator registry mutex. Pending
runtime updates are owned by the instance mailbox and cannot be published after
the instance is removed from the registry. The DSP chain is destroyed only
after unregistering the instance.

Telemetry is the third analysis channel. `DspChain` completes fixed-size level
accumulators at the end of each audio block and publishes a `dsp::TelemetrySnapshot`
through `AnalysisSlot::Telemetry()`. The snapshot contains four chain meter
points, compressor gain reduction, and saturator distortion. No worker handles
this path. Only the slot belonging to the current `AnalysisView` enables
telemetry collection; other instances skip both chain accumulators and
processor-specific telemetry accumulation. Audio uses the lock-free
`LatestSnapshot` transport and the UI reads only the newest block during
`analysis_tick`. Max emits `meter`,
`saturator_distortion`, and `compressor_reduction` frames through the existing
`analysisOutput` outlet. Native smoothing uses a 150 ms time constant and the
actual block duration (`frameCount / sampleRate`), so its response is
independent of the host buffer size. Level RMS is smoothed in linear amplitude
and converted to dB only for the published snapshot; block peak remains an
independent instantaneous dB value. Compressor reduction is accumulated as
linear attenuation: RMS attenuation and minimum attenuation are converted to
positive reduction dB only for the published UI snapshot.
The compressor smoothed reduction field is then time-smoothed directly in
positive dB and starts from neutral `0 dB`.
Telemetry keeps a local source `revision` per DSP instance and receives the
global analysis `viewRevision` when read by `AnalysisService`. Consumers compare
their local cursor only within the same view revision, so switching instances
cannot hide a newer view whose local revision is smaller.
