# DSP snapshot exchange

## Managed to Native: DSP State

Each native host instance owns one `DspStateExchange` and passes its pointer during
instance registration. The exchange has three parameter snapshots,
`publishedIndex`, `consumerIndex`, and a separate native-written telemetry
snapshot. Managed reads the published and consumer-protected parameter slots,
writes the only remaining slot, and publishes its index with `Volatile.Write`.
The publisher serializes this read-select-write-publish sequence with a
Managed control-side lock, so one publisher has a single writer even when
control events arrive on multiple Managed threads. The lock is never used by
the native audio thread.
The publisher also has a lifecycle gate. `InstanceRegistry.UnregisterInstance`
waits for an active publish, marks the publisher stopped, and clears its exchange
pointer before `UnregisterInstance` returns. Later publishes are ignored, so
the native host instance can destroy its exchange after the unregister barrier.
The native audio callback reads the published index with acquire ordering,
claims it as `consumerIndex` with release ordering, and copies it once into the
native-local DSP state. Managed never writes the published or consumer-protected
slot, so
the payload copy has explicit ownership and does not require a retry loop.
If `publishedIndex` equals the native local reading index, there is no new
snapshot. The exchange remains a POD layout; C++ applies `std::atomic_ref` to
its two publication fields.
This consume path is independent of protocol callbacks and message-thread draining. Initial state and changes for inactive presentation instances therefore
reach DSP on the next audio block even when no UI frame is delivered.

`DspSnapshot` contains the current scalar runtime controls for input/output
level, saturator, compressor, Polish, and equalizer state, together with fixed
structured slots for equalizer and detector filters. Each slot carries activity,
filter type, frequency, gain, Q and fixed Q; parameters not supported by a
filter type are zero. The per-instance `StateTree` is authoritative; its value
observers maintain the small `DspRuntimeState` projection from which the
fixed-layout snapshot is published. Boolean markers use `uint32` values in the
ABI (`0` or `1`) so the C# and C++ layouts remain explicit and blittable. The
snapshot carries raw filter data, not compiled coefficients; Native DSP
compilation remains a later consumer of these slots.
Snapshot structs have no domain defaults. Managed StateValueDefinitions/model construction creates initial parameter state before the first publish.
`Prepare(sampleRate, maximumFrameCount)` updates the native DSP compilation
context and the Managed analyzer context after audio configuration; it must not
reset or republish parameter state. Native creates its per-instance DSP graph
and its bounded parallel branch storage during this control/setup operation.
The graph itself is audio-owned and is not swapped on parameter publications;
processor history and smoothing state survive every accepted snapshot.
Parameter publication and telemetry use opposite ownership directions and
independent synchronization fields inside the fixed exchange. The complete
telemetry contract is described in [`DspTelemetry.md`](../../Consolidator.Managed/Docs/Aspects/Telemetry.md).


Exact layout and the Gain Staging extension are defined in [GainStaging](../../Consolidator.Managed/Docs/Aspects/GainStaging.md); state compilation is in [StatePipeline](../../Consolidator.Managed/Docs/Aspects/StatePipeline.md).
