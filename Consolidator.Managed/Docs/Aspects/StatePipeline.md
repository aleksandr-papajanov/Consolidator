# Managed DSP state projection

## Ownership

The Managed state tree is the only source of truth for user parameters. Value
observers project committed changes into the per-instance `DspRuntimeState`, and
the resulting fixed-layout `DspSnapshot` is published to Native. The snapshot
also carries fixed structured slots for equalizer and detector filter type,
activity, frequency, gain, Q and fixed Q. The audio callback reads only that
published snapshot.

Slow native runtime values that exist only for live control presentation return
through the separate latest-only [`DspTelemetry.md`](Telemetry.md) channel.
They do not become authoritative state until an explicit control operation
commits them.

Managed does not maintain a second analyzer copy of filter state, coefficient
caches, dirty-bank maps, or rendered curve arrays. Coefficient and response
calculation used only for drawing belongs to VST3 presentation.

Bank approximation performs separate Managed control-path calculations from authoritative state. It neither owns a presentation curve cache nor consumes streamed analyzer FFT. See [Approximation](Approximation.md) for the concrete calculation and atomic Commit/Refine behavior.

## Related boundaries

Spectrum capture and FFT: [SpectrumAnalysis](SpectrumAnalysis.md).
Frontend curves and preview: [GraphEditing](../../../Consolidator.Vst3/Docs/Aspects/GraphEditing.md).
Snapshot publication/consumption: [DspStateExchange](../../../Docs/Aspects/DspStateExchange.md).
Audio-owned smoothing and graph history: [Native Runtime](../../../Consolidator.Native/Docs/Aspects/Runtime.md).

## DSP projection

DSP state constructors attach `StateProjectionObserver<TValue>` directly to
the values that affect the runtime snapshot. The observer applies the initial
value and every later effective change. Bypass observers update both the raw
bypass marker and its derived active flag; and equalizer
bank/filter activity are projected by their owning state objects.

These observers run only on the Managed control path. Native audio reads the
published fixed-layout snapshot and never accesses observers or the state tree.
