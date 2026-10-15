# Managed runtime and instance lifecycle

## Instance Registration

The native host instance registers an instance with a callback and context:

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

The context is the native host instance. A null output callback is rejected by managed registration and produces instance ID `0`.
The DSP exchange pointer is required and points to memory owned by the native host instance for the lifetime of the registered instance. Registration also returns an opaque `audioInputHandle` for the native audio path. Managed allocates one `NativeAudioInput` for the instance and tracks its handle by `InstanceId` for control-path cleanup; Native stores the returned handle for audio calls.

`ManagedBridge` instances share one native `ManagedRuntime`. The runtime lazily
loads `Consolidator.Managed.dll` and resolves its exported functions once per
process. NativeAOT libraries do not support `FreeLibrary`, so the runtime keeps
the module loaded until process termination. It owns the Managed log callback
registration and clears that callback when the last host instance is destroyed, so
the callback never points into an unloaded native host instance module.
The native bridge counts live host instances and explicitly calls the shared
`ConsolidatorShutdown` export when the last host instance is destroyed. This stops
Managed worker services without unloading the NativeAOT runtime. Opening a new
host instance in the same process installs the log callback again; registration then
creates a fresh Managed service provider.


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

The host unregisters before releasing the exchange, callback context or queue storage. This keeps the callback context valid until the Managed barrier completes.
`UnregisterInstance` takes only the `InstanceId`; Managed uses its instance-to-audio-handle ownership to release the audio handle after the instance has stopped.


Before `UnregisterInstance` starts, the native host must stop starting new
audio callbacks for that host instance. An audio callback that already resolved the
handle may finish while unregister is in progress, but no new
`SendAudio(audioInputHandle)` may begin after handle release. The native
host lifecycle must therefore serialize audio callback start with
destruction; catching an invalid `GCHandle` in Managed is not a lifetime
mechanism.

When the last host instance is destroyed, the native bridge calls
`ConsolidatorShutdown`. Managed disposes the DI provider, which stops and
disposes all remaining instances. Any remaining audio-input handles are then
released and the native bridge clears the log callback. The NativeAOT library
remains mapped until process termination because NativeAOT does not support
dynamic unloading.

