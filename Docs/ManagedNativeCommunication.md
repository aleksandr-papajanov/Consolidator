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

The managed registry stores per-instance `ManagedInstance` records. Each
`ManagedInstance` owns its `ManagedState`, DSP publisher and command gate. The
same instance ID is the delivery identity for that external's control output;
there is no separate UI session.
Output remains globally routed by `NativeOutputService`, exposed as
`IProtocolTransport` to Protocol and `IProtocolOutputRegistry` to Native API.

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
    -> IProtocolTransport.Send
```

The native side owns the encoded input atoms only for the duration of the unmanaged call. `SendMessage` immediately converts the selector and atoms into managed values before returning.

`Receive` is the Managed control-message entry point exposed through
`ProtocolService`. It decodes the incoming frame through `CommandDecoder`,
creates a typed command, executes it through `CommandEndpointRegistry` and
`InstanceCommandRouter`, and sends the result through the source instance output
registry. The registry fans out by `TargetInstanceIds` to native callback
bindings. Instance preparation is exposed through `IInstancePreparationService`; lifecycle
registration is exposed through `IInstanceLifecycleService`. The Managed
protocol architecture is defined in
[`ManagedProtocol.md`](ManagedProtocol.md): command types, handler registrations,
relative path/value decoder boundaries, multipart target/registry snapshots,
and response encoder boundaries are owned by Managed. `initialize` identifies
the calling external. `observe_target(instance, bank)` records its view and
returns a complete UI projection. Subsequent notifications are routed to
observers of that instance or exact bank. The protocol has no session, epoch or
selected-bank field.

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
router: the focused target's `StateValue` and its peer observer perform the
grouped history transaction. There is no second broadcast-write path in the
command executor. Each target instance has
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

`ReceiveManagedOutput` never calls a Max outlet directly. It pushes the owned `OutputFrame` into `pendingOutput_` and schedules one `c74::min::queue<>`:

```text
C# callback thread
    -> copy OutputFrame
    -> lock only for pendingOutput_.push_back
    -> outputQueue_.set()

Max low-priority thread
    -> DrainManagedOutput
    -> swap pendingOutput_
    -> convert OutputFrame to Max atoms
    -> controlOutput.send
```

Repeated `queue.set()` calls coalesce while the qelem is pending. Frames are not coalesced or dropped; only the wake-up request is coalesced.

The mutex is held only while pushing or swapping the queue. String conversion and Max atom construction happen outside the producer-side lock.

The native callback handler is `noexcept` and catches exceptions around `ReceiveManagedOutput`. No C++ exception may cross the C ABI boundary.

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
shared operation lock. The adapter forwards to all DI-registered
`IInstanceAudioInputHandler` implementations. Unregistration releases the
audio handle after removing the instance ID. Audio handlers must remain
real-time safe: no Max outlet calls, blocking locks, or unbounded allocation
on the audio thread.

