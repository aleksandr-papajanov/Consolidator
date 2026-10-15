# C ABI and borrowed data

## ABI Types

The shared ABI is declared in `Consolidator.Native/Transport/ManagedInterop.h` and mirrored by the C# unmanaged types.
DSP runtime structs use natural platform alignment: C# uses
`StructLayout(LayoutKind.Sequential)` without `Pack`, and C++ uses ordinary
struct layout without packing pragmas. ABI size and offset tests document the
resulting contract; packing must not be used to force those values.

`NativeAtom` is a 16-byte value with a type tag and union payload. Tag values are Integer=1, Float=2, Symbol=3. The native encoder does not interpret state paths or commands; Managed owns semantic decoding.

| Type | Value |
| --- | --- |
| `Integer` | signed 64-bit integer |
| `Float` | IEEE 754 `double` |
| `Symbol` | UTF-8 `const char*` / `byte*` |

The exported native entry points use the C calling convention:

- `ConsolidatorSetLogCallback`
- `ConsolidatorRegisterInstance`
- `ConsolidatorUnregisterInstance`
- `ConsolidatorShutdown`
- `ConsolidatorSendMessage`
- `ConsolidatorPrepare`
- `ConsolidatorSendAudio`
- `ConsolidatorCapturePersistence`
- `ConsolidatorFreePersistence`
- `ConsolidatorRestorePersistence`

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
reported through the host log sink instead of failing during static initialization
before logging is available. Provider construction is lazy and publishes the
provider only after a successful build; a failed build therefore does not poison
the Managed services type or disable diagnostics for later registration calls.


## Input atom ownership

The caller owns encoded input atoms for the duration of ConsolidatorSendMessage.
Managed copies selector/symbol data before returning. Semantic command decoding,
queueing and routing are specified in the [Managed protocol](../../Consolidator.Managed/Docs/Aspects/ManagedProtocol.md).
## Managed to Native: Output Callback

Managed code sends output through `NativeOutput.Send`:

```text
IProtocolTransport
    -> NativeOutput.Send
    -> ManagedOutputCallback
    -> host output callback
```

`NativeOutput.Send` accepts a `ProtocolOutput`. It temporarily allocates UTF-8
memory for the selector and symbol atoms, invokes the callback, and frees that
memory in `finally` after the callback returns.

The native callback must copy all borrowed data synchronously. It copies:

- the selector into `std::string`;
- integer and floating-point values by value;
- symbols into `std::string`.

After the callback returns, native code must not retain any pointer received from C#.


## Persistence boundary

The three persistence exports transfer a versioned UTF-8 snapshot. Capture
returns caller-owned memory that remains valid after the export returns; Native
must release it exactly once through `ConsolidatorFreePersistence`. Restore
borrows its input only for the duration of the call and copies it before the
barrier is queued.

Dirty-state delivery, raw host state bytes, schema validation and history baseline are specified in
[`Persistence.md`](../../Consolidator.Managed/Docs/Aspects/Persistence.md).


Registration and teardown: [InstanceLifecycle](InstanceLifecycle.md). Audio contract: [AudioInput](AudioInput.md). Parameter exchange: [DspStateExchange](DspStateExchange.md).
