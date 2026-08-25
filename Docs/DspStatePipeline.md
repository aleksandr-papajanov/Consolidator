# Managed DSP State Pipeline

## Ownership

The state tree remains the only source of truth for user parameters. Managed DSP
objects are derived objects owned by a per-instance `AnalyzerRegistry`; they do not
replace `StateValue` objects and do not expose alternate parameter APIs.

Each instance registry contains one DSP instance object with:

- the instance-level DSP objects;
- lazily-created equalizer bank cache objects;
- lazily-created biquad objects for demanded equalizer banks;
- detector filter objects owned by the corresponding detector DSP object.

A biquad object stores its current derived coefficients. Its input is the current
`FilterState` values, not a copied second state tree. The registry receives the
actual audio sample rate from `InstancePreparationService`; a newly registered
instance starts at 48 kHz until preparation provides its real rate.

## Dirty propagation

`StateValueFactory` attaches one DSP observer to every application state value.
The observer classifies the state path and marks the narrowest affected DSP node:

```text
filter.frequency / filter.q / filter.gain
  -> filter dirty
  -> equalizer bank dirty
  -> instance DSP dirty
```

Bypass and topology changes mark the owning filter or bank as dirty when they
change the rendered response. Parameters that only affect the native runtime
snapshot continue to use their existing projection observers and do not trigger
biquad calculation.

The observer resolves the concrete bank and filter directly from the path.
There is no global dirty walk: an unchanged value produces no notification,
and a changed value invalidates only its owning filter and bank response.

## Point updates

`DspStateObserver` receives the concrete changed `StatePath` and marks its
analyzer address pending. At the end of the serialized control operation the
registry captures one immutable input for every distinct pending address and
marks it dirty with a new revision. It does not calculate coefficients or
curves. The analyzer worker removes the latest dirty entry and requests a
presentation only if no newer revision superseded it. Curve publication first
checks for focused recipients; only then does it materialize the invalidated
bank from its latest immutable input. Hidden peer instances therefore keep
correct inputs without paying for unused curve calculations.

A transaction commits all pending state slots before its committed-change
observers run, so observers see the final values even when several leaves were
changed together. Repeated updates coalesce in the same dirty map; therefore a
frequency and gain update for one filter, or a series of mouse-move events,
produces at most one focused calculation for the latest bank input.

The analyzer worker consumes at most one queued FFT window every 33 ms. Only
the source selected by the single active viewer owns a capture queue, so adding
loaded instances cannot multiply FFT calculation or UI-frame throughput. When
that source has accumulated more than one window, analysis skips directly to
its newest complete window instead of rendering stale spectrum history.

Registration creates state and runtime objects but does not build analyzer
curves. It captures the initial immutable filter inputs once; the first active
analyzer presentation materializes the required bank caches from those inputs
and never reads the live state tree on the analyzer worker. Sample-rate changes
rebuild only materialized caches; ordinary unchanged writes do not notify the
observer and do no DSP work.

FFT capture and curve delivery follow the single Live-selected device. Max
sends `instance_active 1|0` when selection changes; activating one external
replaces the previous active viewer. The capture belongs to the source selected
by that viewer, even when it is a remote instance. Activation immediately
publishes the current curves. Loaded but inactive instances therefore cannot
multiply worker or UI-frame throughput. Dirty curve calculation and delivery coalesce to
the 33 ms UI cadence while retaining the latest dirty input. Each interval
publishes at most two oldest dirty curve addresses with active recipients,
imposing one global output budget independent of instance count while
preventing a continuously edited source from starving older visible sources.

Grouped writes still capture the latest filter input for every affected source,
but dirty addresses without an active focused recipient are removed before
budget accounting. They therefore cannot delay the visible curve. Selecting
such a source later materializes its curve directly from the retained latest
input.

Switching banks within the same source instance publishes only the selected
equalizer-bank curve. Detector curves are instance-owned and are replayed only
when the observed source instance changes.

## Focus and topology

DSP snapshot projection is independent of UI focus. Changed filters invalidate
their owning analyzer banks, while the expensive curve is recalculated only
when the active viewer needs its presentation. Analyzer notifications are sent
only when the changed source and bank match that viewer's current focus.

A grouped bank write may dirty several instance/bank objects. The registry
coalesces those addresses before the flush; focus determines both notification
recipients and whether a bank curve is recalculated.

Dirty observation records only the affected analyzer address. The registry
captures the latest complete filter state once at the control-operation flush;
intermediate observer callbacks never allocate filter snapshots.

Equalizer bank bypass is part of the bank input and invalidates the same bank
address as its filters, so active state, bank curve, and all-bank curve are
published from one coherent snapshot.

The all-bank equalizer response is cached per source and invalidated by any
equalizer-bank change or sample-rate change. Detector presentations reuse one
immutable unity all-bank curve instead of allocating it for every frame.
Detector filters keep one instance-level state and curve cache regardless of
the selected equalizer bank. Like other instance-owned controls, an edit uses
the selected bank only to choose its grouped peer instances.

Changing focus does not dirty coefficients. For analysis it only changes which
already-built bank response is selected; for controls it rebuilds the
directional instance-owned peer ranges. A topology change rebuilds peer
relationships first and then invalidates affected derived DSP views once.

## Threading and publication

State-tree reads and dirty-map updates run on the Managed control path under the
existing operation gate. Coefficient and curve calculation run on the analyzer
worker from immutable inputs and never read the state tree. The audio callback
never accesses the registry, state tree, or observers. Native continues to
consume only the fixed `DspSnapshot` through the existing publisher.

DSP publication is driven by committed-change observers rather than the
command's original target list. At the end of a control operation Managed
drains the distinct instance ids whose projected DSP state actually changed.
This includes peer edits in connected instances and indirect `Audible` changes
caused by mute, solo, registration, or removal. Every affected instance is
published once.

Analyzer response data is a separate analysis-output contract from the native
snapshot and control output. Equalizer and detector filters use the same
Managed curve cache and response calculation. The resulting view-specific
frames are routed through the dedicated Max `analysisOutput` outlet; Max does
not calculate detector responses.

Max transports handle presentations separately from streamed curve and
spectrum updates. Frequency/gain state notifications therefore update only the
handles; they do not replay the existing 256-point arrays. The analyzer control
retains its last streamed arrays across a handle presentation and coalesces
redraw requests to its 33 ms UI cadence. Each handle presentation also carries
normalized effective frequency/gain limits, so drag preview and outgoing
values stop at the current peer intersection.
