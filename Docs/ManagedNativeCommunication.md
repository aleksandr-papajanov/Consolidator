# Managed and Native Communication

This document describes the current communication contract inside `InteropSandbox` between the C# managed core, the C++ native bridge, and the Max external.

## Components

```text
Max external
    |
    | ManagedBridge
    v
Consolidator.Managed.dll
    |
    | NativeApi / Managed boundary interfaces
    v
Managed instance registry
```

## Managed to Native: DSP State

Each native external owns one `SharedDspExchange` and passes its pointer during
instance registration. The exchange has three snapshots plus `publishedIndex`,
and `consumerIndex`. Managed reads the published and consumer-protected slots,
writes the only remaining slot, and publishes its index with `Volatile.Write`.
The publisher serializes this read-select-write-publish sequence with a
Managed control-side lock, so one publisher has a single writer even when
control events arrive on multiple Managed threads. The lock is never used by
the native audio thread.
The publisher also has a lifecycle gate. `InstanceRegistry.UnregisterInstance`
waits for an active publish, marks the publisher stopped, and clears its exchange
pointer before `UnregisterInstance` returns. Later publishes are ignored, so
the native external can destroy its exchange after the unregister barrier.
The native audio callback reads the published index with acquire ordering,
claims it as `consumerIndex` with release ordering, and copies it once into the
native-local DSP state. Managed never writes the published or consumer-protected
slot, so
the payload copy has explicit ownership and does not require a retry loop.
If `publishedIndex` equals the native local reading index, there is no new
snapshot. The exchange remains a POD layout; C++ applies `std::atomic_ref` to
its two publication fields.

`DspSnapshot` contains the current scalar runtime controls for input/output
gain, saturator, compressor, and equalizer state. The per-instance `StateTree`
is authoritative; its value observers maintain the small `DspRuntimeState` projection
from which the fixed-layout snapshot is published. Boolean markers use
`uint32` values in the ABI (`0` or `1`) so the
C# and C++ layouts remain explicit and blittable. Filter-bank values and
compiled coefficients are a separate future extension of this snapshot.
Snapshot structs have no domain defaults. The initial `gain = 1.0` state is
created by Managed `DspDefaults` and compiled before the first publish.
`Prepare(sampleRate, maximumFrameCount)` is reserved for updating the DSP
compilation context after audio configuration; it must not reset or republish
parameter state. Managed writes the exchange and native reads it, so telemetry
must use a separate native-owned channel.

Each Max external owns one managed instance. The instance ID addresses all control and audio calls for that external.

Observed target and instance activity are separate pieces of state. The
`observe_target` command changes only the instance/bank shown by the UI.
`set_instance_active 1|0` reports whether the source external is Live's single
selected device; activating one external replaces the previous active viewer.
The capture buffer belongs to the source selected by that viewer, which may be
a different instance. Without an active viewer, the audio entrypoint returns
before capture lookup or audio copying. The analyzer worker processes at most
one FFT window per 33 ms. Equalizer curve invalidation remains a control-side
update; publication is limited to the active viewer and has its own budget.

The Max client treats each `target_state_snapshot` as one target transition.
Bindings are suspended before `observe_target` is sent. The client assembles
the complete frame outside its presentation cache, replaces that cache once,
and then resumes bindings with the latest presentation. State value view models
keep the previous target while the request is pending, so loading is exposed as
one target-level transition rather than repeated per-value state changes.
Responses are correlated by generation/request ID; a stale response cannot
resume bindings or replace the current target. A successful snapshot implies
`ready` for all entries, and an error applies to the snapshot as a whole.

The snapshot frame is:

```text
target_state_snapshot 1 source requestId instanceId bankId entryCount
    path value physicalMin physicalMax effectiveMin effectiveMax × entryCount
```

Curve presentation refreshes are queued as latest-only analyzer work. Changing
the focused bank updates the active source and capture demand, then returns;
curve calculation and publication happen on the analyzer worker and never delay
the target snapshot.
During a local dial gesture, the control renders its local preview until the
gesture ends. Authoritative `state_changed` presentations continue updating the
stored value but do not replace the value currently under the pointer.

## ABI Types

The shared ABI is declared in `Consolidator.Native/External/ManagedInterop.h` and mirrored by the C# unmanaged types.
DSP runtime structs use natural platform alignment: C# uses
`StructLayout(LayoutKind.Sequential)` without `Pack`, and C++ uses ordinary
struct layout without packing pragmas. ABI size and offset tests document the
resulting contract; packing must not be used to force those values.

`NativeAtom` is a 16-byte value with a type tag and a union payload:

| Type | Value |
| --- | --- |
| `Integer` | signed 64-bit integer |
| `Float` | IEEE 754 `double` |
| `Symbol` | UTF-8 `const char*` / `byte*` |

The exported native entry points use the C calling convention:

- `ConsolidatorSetLogCallback`
- `ConsolidatorRegisterInstance`
- `ConsolidatorUnregisterInstance`
- `ConsolidatorSendMessage`
- `ConsolidatorPrepare`
- `ConsolidatorSendAudio`

Every `[UnmanagedCallersOnly]` entrypoint catches exceptions before returning to
native code. Registration returns `0` after a boundary failure; control-path
void entrypoints report failures through the Managed log sink and return. The
audio entrypoint does not format or log exceptions on the realtime thread: it
increments an atomic counter, which a later control-path entrypoint drains and
reports. The boundary logger also absorbs its own failures, so no managed
exception can cross the C ABI.

The process-wide native log sink exists independently of the application service
provider and is registered into that provider as the logging singleton. The log
callback is therefore configured before the provider is built or any application
service is resolved. Managed API services are resolved inside their corresponding
entrypoints, so a dependency-injection failure during instance registration is
reported to the Max console instead of failing during static initialization
before logging is available. Provider construction is lazy and publishes the
provider only after a successful build; a failed build therefore does not poison
the Managed services type or disable diagnostics for later registration calls.

## Instance Registration

The native external registers an instance with a callback and context:

```text
ConsolidatorRegisterInstance(
    context,
    outputCallback,
    dspExchange,
    audioInputHandle)
        -> instanceId
```

The callback has this shape:

```text
(context, selector, atoms, atomCount)
```

The context is the native external instance. A null output callback is rejected by managed registration and produces instance ID `0`.
The DSP exchange pointer is required and points to memory owned by the native external for the lifetime of the registered instance. Registration also returns an opaque `audioInputHandle` for the native audio path. Managed allocates one `NativeAudioInput` for the instance and tracks its handle by `InstanceId` for control-path cleanup; Native stores the returned handle for audio calls.

`ManagedBridge` instances share one native `ManagedRuntime`. The runtime lazily
loads `Consolidator.Managed.dll` and resolves its exported functions once per
native module. It owns the process-wide Managed log callback registration and
clears that callback before releasing the DLL in its destructor. Destroying one
external therefore cannot unload the ManagedAOT module while other externals
or the shared Managed services are still active.
The native bridge counts live external objects and explicitly calls the shared
`ConsolidatorShutdown` export when the last external is destroyed. This stops
Managed worker services before the ManagedAOT module is released; the runtime
destructor remains an idempotent final safety net.

The managed registry stores per-instance `ManagedInstance` records. Each
`ManagedInstance` owns its `ManagedState`, DSP publisher and command gate. The
same instance ID is the delivery identity for that external's control output;
there is no separate UI session.
`NativeOutputService` remains the direct output transport for protocol responses
and the callback registry. Presentation publishers use a separate
`PresentationOutputGate`: it keeps only per-instance active state. Active
instances receive entries immediately, while inactive `ActivePresentation`
entries are discarded and never enter an outgoing backlog. After activation is
acknowledged, the Max host
requests one current `target_state_snapshot` and resumes bindings only after it
has atomically replaced the target cache. Lossless outputs and `LatestAnalysis`
outputs continue directly through the gate.
Unregistration removes the gate state before the native callback is removed.
Presentation publishers provide explicit recipient sets for each operation.
Targeted state writes and resets publish DSP snapshots only to their affected
instance IDs. Registry deltas and history notifications are sent only to
instances that have requested a registry snapshot and are therefore registered
as registry observers. The Max registry client requests a snapshot and keeps a
registry observer only while its UI instance is active; deactivation removes
that demand, so inactive UIs neither fetch nor accumulate registry deltas.
Activation performs one current snapshot fetch before resuming delta delivery.
State notifications use topology-resolved focused
observers; equalizer curves use observers of the exact `(instance, bank)`; FFT
frames return the selected source to the active viewer. A singleton
service does not imply broadcast delivery.

Runtime metrics expose `presentation_active_deliveries` and
`presentation_discarded` for active and inactive state entries respectively.
`native_control_frames` reports the number
of Managed native input frames and is the before/after comparison point for
grouped edits. With fourteen devices and one active recipient, a grouped drag
should produce one active `state_changed` delivery while inactive presentation
entries increase only `presentation_discarded` and never create pending state.

## Native to Managed: Incoming Messages

A Max control message follows this path:

```text
Max control inlet
    -> ConsolidatorExternal::ForwardMessage
    -> AtomCodec::Encode
    -> ManagedBridge::SendManagedMessage
    -> ConsolidatorSendMessage
    -> NativeApi.SendMessage
    -> AtomDecoder.Decode
    -> ProtocolService.Receive
    -> bounded control queue
    -> single Managed control worker
    -> IProtocolTransport.Send
```

The native side owns the encoded input atoms only for the duration of the unmanaged call. `SendMessage` immediately converts the selector and atoms into managed values before returning.

`Receive` is the Managed control-message entry point exposed through
`ProtocolService`. It decodes and copies the incoming frame through
`CommandDecoder`, then performs a non-blocking bounded enqueue and returns to
Native. A single Managed control worker consumes the FIFO queue, executes each
command through `CommandEndpointRegistry` and `InstanceCommandRouter`, and
sends the result through the source instance output registry. This keeps
`OperationGate` waits off the Max message thread. When the queue is full, the
caller receives an `error overloaded` response. Unregister marks the source as
cancelled so queued commands for that instance are discarded before execution.
Instance preparation is exposed through `IInstancePreparationService`; lifecycle
registration is exposed through `IInstanceLifecycleService`. The Managed
protocol architecture is defined in
[`ManagedProtocol.md`](ManagedProtocol.md): command types, handler registrations,
relative path/value decoder boundaries, multipart target/registry snapshots,
and response encoder boundaries are owned by Managed. `initialize` identifies
the calling external. `observe_target(instance, bank)` records its view and
returns a complete UI projection. Subsequent notifications are routed to
observers of that instance or exact bank. The protocol has no session, epoch or
selected-bank field.

The NativeAOT `SendMessage` boundary records only aggregate call count and
elapsed time in `RuntimeMetrics`. Ordinary incoming messages are not formatted
or sent to the native log sink. The `metrics` command is the explicit way to
report this telemetry; errors continue to use the native log sink.

## Managed Instance Commands

The first managed command contract is intentionally separate from atom decoding
and command construction. An external caller creates a `ReadStateCommand` or a
`WriteStateCommand` and submits it through the registered command execution
endpoint, which routes and executes it through `CommandExecutor`.
with the source instance ID. A read and `WriteStateCommand` resolve exactly the
instance explicitly addressed by the UI frame, or the source external's
currently observed `BankAddress` for relative internal commands. The write
is then propagated by the target StateValue's materialized peer registration
when its scope requires it.

The command is data-only and implements `IInstanceCommand<TResult>`. The
executor resolves a DI-registered
`ICommandHandler` by command type and invokes it with an
`InstanceCommandContext` containing the target instance ID and state. The router returns
`ValueTask<CommandExecutionResult<TResult>>`. A read returns one typed value.
A write returns one aggregate result with `TargetCount`, `AppliedCount`, and an
optional error. There is no public incoming or outgoing command queue.

The shared operation gate covers one complete logical operation. Synchronous
topology/history operations and async command batches use the same gate, so a
command cannot interleave with a history jump or a topology mutation while it
is suspended. `WriteStateCommand` does not broadcast through the command
router. The router keeps the source external's focused `BankAddress` active for
the complete operation, and the target `StateValue` peer observer uses that
context to prepare the grouped history transaction. Bank-owned peers remain
materialized by bank address; instance-owned peers are resolved from an
address index for the selected bank's group. There is no second broadcast-write
path in the command executor. Each target instance has
its own async execution gate for non-broadcast commands. Cancellation
propagates through the command and is not converted into a regular execution
error. The protocol decoding and result formatting boundaries are defined by
`ManagedProtocol.md`.

## Managed to Native: Output Callback

Managed code sends output through `NativeOutput.Send`:

```text
IProtocolTransport
    -> NativeOutput.Send
    -> ManagedOutputCallback
    -> ConsolidatorExternal::ReceiveManagedOutput
```

`NativeOutput.Send` accepts a `ProtocolOutput`. It temporarily allocates UTF-8
memory for the selector and symbol atoms, invokes the callback, and frees that
memory in `finally` after the callback returns.

The native callback must copy all borrowed data synchronously. `ReceiveManagedOutput` copies:

- the selector into `std::string`;
- integer and floating-point values by value;
- symbols into `std::string`.

After the callback returns, native code must not retain any pointer received from C#.

## Native Queue and Max Thread

`ReceiveManagedOutput` never calls a Max outlet directly. All control frames,
including `state_changed`, are stored in one FIFO so state notifications retain
their order relative to target snapshots, history notifications, errors, and
command responses. Analysis frames use latest-only slots: one pending `fft` frame and one pending
curve frame per analyzer selector and native external. A newer frame for the
same selector replaces the older one before the Max queue is drained.

One `c74::min::queue<>` is scheduled for both paths:

```text
C# callback thread
    -> copy OutputFrame
    -> lock only for control enqueue or latest-only slot replacement
    -> outputQueue_.set()

Max low-priority thread
    -> DrainManagedOutput
    -> drain a bounded control batch
    -> send the latest analysis frames
    -> convert OutputFrame to Max atoms
    -> controlOutput.send or analysisOutput.send
```

Analysis frames use the dedicated `analysisOutput` outlet. The native drain
routes `fft`, `equalizer_curves`, `compressor_detector_curves`, and
`saturator_detector_curves` there; other Managed protocol frames use
`controlOutput`. The Max bridge connects that analysis outlet to the UI host's
protocol input separately from control output.

Each drain processes at most 32 lossless control frames and 4096 control atoms,
then processes the latest available analysis frames. If any queue or slot still has work, the qelem is
scheduled again. Repeated `queue.set()` calls coalesce while the qelem is
pending.

The mutex is held only while pushing or swapping the queue. String conversion and Max atom construction happen outside the producer-side lock.

The native callback handler is `noexcept` and catches exceptions around `ReceiveManagedOutput`. No C++ exception may cross the C ABI boundary.

## Runtime Metrics

Sending the `metrics` control message requests a diagnostic snapshot. Managed
reports native-input and control-operation time, FFT and curve rates per
instance, equalizer calculation time, registry snapshot/delta counts, dropped
audio blocks and process Managed allocation totals through the control-path log sink. Native
reports current control FIFO depth, replaced and skipped FFT frames, and the
most recent Max queue drain duration. Audio-path metric updates use atomic
counters only; they do not log or allocate.

`ConsolidatorUiHost` sends diagnostic snapshots around UI activity. Continuous
gestures report once at `gestureBegan` and once at `gestureEnded`; intermediate
`valueChanged` and `filterMoved` intents do not emit metrics. Discrete button,
reset, selection and bank-manager intents emit one snapshot after dispatch. This
keeps diagnostics correlated with user activity without continuously loading the
Max scheduler or flooding the console during a drag.

Instance activity is also the presentation lifecycle boundary. Inactive UI hosts
do not receive state presentation and keep their control bindings
disabled. Activating an instance first publishes `set_instance_active`, then
requests a current target snapshot. Bindings resume only after the complete
snapshot has atomically replaced the client cache. Bank manager activation
likewise requests a complete registry snapshot; analyzer activation restores the
latest handles, spectrum and curves.

## Lifecycle Contract

Registration enables callbacks for the instance:

```text
Register
    -> instance active
    -> callbacks allowed
```

Unregistration is synchronous:

```text
UnregisterInstance(id)
    -> remove instance from registry
    -> reject new producers for that instance
    -> wait for an active TrySend callback
    -> wait for an active command operation
    -> mark instance inactive
    -> dispose state and command gate
    -> return with callbacks impossible
```

`IProtocolOutputRegistry` removes the instance callback binding before unregister returns.
The native callback binding remains responsible for serializing an active
callback with its own native lifetime.

The native destructor calls `UnregisterInstance` before destroying the external, then clears `instanceId_` and unsets the qelem. This makes the native `this` context valid until the managed callback barrier has completed.
`UnregisterInstance` takes only the `InstanceId`; Managed uses its instance-to-audio-handle ownership to release the audio handle after the instance has stopped.
Before `UnregisterInstance` starts, the native host must stop starting new
audio callbacks for that external. An audio callback that already resolved the
handle may finish while unregister is in progress, but no new
`SendAudio(audioInputHandle)` may begin after handle release. The native
external lifecycle must therefore serialize audio callback start with
destruction; catching an invalid `GCHandle` in Managed is not a lifetime
mechanism.

Before unloading `Consolidator.Managed.dll`, the native bridge calls
`ConsolidatorShutdown`. Managed disposes the DI provider, which stops and
disposes all remaining instances. Any remaining audio-input handles are then
released, the log callback is cleared, and only then is the library unloaded.

## Audio Calls

Audio setup and audio input currently use direct managed entry points:

```text
ConsolidatorExternal::Prepare
    -> ConsolidatorPrepare
    -> IInstancePreparationService.Prepare

`ConsolidatorExternal::operator()`
    -> ConsolidatorSendAudio(audioInputHandle)
    -> NativeAudioInput.ReceiveAudio
    -> IInstanceAudioInputService.ReceiveAudio
```

`SendAudio` resolves the opaque `GCHandle` directly to the per-instance
`NativeAudioInput`; it does not query the instance registry or take the
shared operation lock. The adapter forwards to the singleton
`FftAnalyzer`. Unregistration releases the audio handle after removing the
instance ID. Audio handlers must remain real-time safe: no Max outlet calls,
blocking locks, or unbounded allocation on the audio thread.

The singleton `FftAnalyzer` is the audio input implementation. It writes the
four input channels into a preallocated bounded SPSC sample ring on the
callback and performs FFT on a managed worker. The ring accepts arbitrary audio
vector sizes, drops newest samples on overflow, and counts dropped samples
atomically. The worker reads complete 1024-sample windows with a 512-sample
hop and applies a Hann window before FFT, so vector boundaries do not discard
the remainder of an input block. It publishes `fft` output only to the active
viewer of the selected source instance; the output contains protocol version,
source instance ID, FFT size, main spectrum bins and reference spectrum bins.

Analyzer EQ caches are lazy. Instance registration creates the state and native
DSP runtime immediately, while bank curves and all-bank presentation are built
only for the active instance viewer. Instance activity gates both spectrum
capture and curve delivery; activation publishes the latest presentation, and
dirty curves are coalesced to the UI cadence before crossing the callback. The
worker publishes at most two oldest dirty curve addresses with active focused
recipients per interval. Dirty addresses without recipients are discarded from
the notification queue without consuming that budget; their latest immutable
inputs remain available for a later focus presentation. The callback rate is
therefore bounded without allowing invisible grouped peers to delay the visible
curve.

Equalizer curve frames are routed to the active viewer when it observes the
exact source instance and bank. Compressor and saturator detector curve frames
are routed to that viewer when it observes the source instance, independent of
its selected EQ bank; detector caches use a single instance-level bank slot.

