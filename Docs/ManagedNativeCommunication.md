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
    | NativeApi / ConsolidatorCore
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
The publisher also has a lifecycle gate. `ConsolidatorInstance.Stop()` waits
for an active publish, marks the publisher stopped, and clears its exchange
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

The prototype snapshot contains only `gain`. It is a derived runtime
representation, not authoritative application state. Managed `InstanceState`
holds the source values and `DspStateCompiler` derives the fixed-layout
snapshot; future compressor and filter coefficients belong in that snapshot.
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

## Instance Registration

The native external registers an instance with a callback and context:

```text
ConsolidatorRegisterInstance(
    context,
    outputCallback,
    dspExchange)
        -> instanceId
```

The callback has this shape:

```text
(context, selector, atoms, atomCount)
```

The context is the native external instance. A null output callback is rejected by managed registration and produces instance ID `0`.
The DSP exchange pointer is required and points to memory owned by the native external for the lifetime of the registered instance.

`ManagedBridge` instances share one native `ManagedRuntime`. The runtime lazily
loads `Consolidator.Managed.dll` and resolves its exported functions once per
native module. It owns the process-wide Managed log callback registration and
clears that callback before releasing the DLL in its destructor. Destroying one
external therefore cannot unload the ManagedAOT module while other externals
or the shared Managed Coordinator are still active.

The managed registry stores a `ConsolidatorInstance`. Its `NativeOutput` is private and can only be used through the instance lifecycle gate.

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
    -> ConsolidatorCore.ReceiveMessage
```

The native side owns the encoded input atoms only for the duration of the unmanaged call. `SendMessage` immediately converts the selector and atoms into managed values before returning.

`ReceiveMessage` currently only establishes the managed control-message routing
boundary. Domain handling and output routing will be added by the Coordinator
handler/router; incoming messages are not echoed back to native.

## Managed to Native: Output Callback

Managed code sends output through `NativeOutput.Send`:

```text
ConsolidatorCore
    -> ConsolidatorInstance.TrySend
    -> NativeOutput.Send
    -> ManagedOutputCallback
    -> ConsolidatorExternal::ReceiveManagedOutput
```

`NativeOutput.Send` accepts `ReadOnlySpan<Atom>`. It temporarily allocates UTF-8 memory for the selector and symbol atoms, invokes the callback, and frees that memory in `finally` after the callback returns.

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
    -> mark instance inactive
    -> return with callbacks impossible
```

`ConsolidatorInstance.TrySend` holds its per-instance lifecycle lock for the callback. `Stop` takes the same lock, so it cannot complete until a callback already in progress has returned. Producers that look up the instance after registry removal cannot obtain it.

The native destructor calls `UnregisterInstance` before destroying the external, then clears `instanceId_` and unsets the qelem. This makes the native `this` context valid until the managed callback barrier has completed.

## Audio Calls

Audio setup and audio input currently use direct managed entry points:

```text
ConsolidatorExternal::Prepare
    -> ConsolidatorPrepare
    -> ConsolidatorCore.Prepare

ConsolidatorExternal::operator()
    -> ConsolidatorSendAudio
    -> ConsolidatorCore.ReceiveAudio
```

`ReceiveAudio` is currently a placeholder. Audio callbacks must remain real-time safe when processing is added: no Max outlet calls, blocking locks, or unbounded allocation on the audio thread.
