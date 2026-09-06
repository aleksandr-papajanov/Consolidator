# DSP state and analyzer presentation

## Ownership

The Managed state tree is the only source of truth for user parameters. Value
observers project committed changes into the per-instance `DspRuntimeState`, and
the resulting fixed-layout `DspSnapshot` is published to Native. The snapshot
also carries fixed structured slots for equalizer and detector filter type,
activity, frequency, gain, Q and fixed Q. The audio callback reads only that
published snapshot.

Slow native runtime values that exist only for live control presentation return
through the separate latest-only [`DspTelemetry.md`](DspTelemetry.md) channel.
They do not become authoritative state until an explicit control operation
commits them.

Managed does not maintain a second analyzer copy of filter state, coefficient
caches, dirty-bank maps, or rendered curve arrays. Coefficient and response
calculation that exists only to draw the Max UI belongs to the JavaScript
`AnalyzerPresenter`.

## Curve presentation

The presenter builds the focused equalizer or detector response directly from
the current presentation bindings. It calculates one biquad coefficient set per
filter and evaluates that set over the shared 256-point logarithmic frequency
grid. The combined response sums the already-calculated signed dB values of the
active filters before normalization.

There is one production curve path. Managed does not publish
`equalizer_curves`, `compressor_detector_curves`, or
`saturator_detector_curves`, and Native has no curve-specific queue slots.

Drag interaction is optimistic. `filterPreview` updates the presenter
immediately, while authoritative writes continue at the bounded gesture rate.
Committed notifications received during the active gesture update the
authoritative bindings without clearing its newer local preview. Gesture end
sends one non-coalesced final position write; the preview is cleared after that
write's response, which follows its state notifications in the lossless control
FIFO. The preceding move already records that position inside the history
transaction, while the final acknowledgement write uses transaction ID zero so
successful completion is not suppressed. A rejected transaction or final write
also clears the preview and rebuilds the presentation from accepted state.

In rough precision mode, the analyzer renders the same snapping grid used by
filter markers: logarithmic frequency positions at ten steps per decade and
gain positions at 3 dB intervals. The grid is presentation metadata generated
from the analyzer scale, so the points disappear in precise mode and remain
aligned with interior marker destinations. Effective group limits still clamp
markers at their boundary values.

The analyzer frequency response is calculated entirely in JavaScript from the
focused parameter bindings and local filter definitions. Managed publishes no
analyzer configuration, raw equalizer state, or filter catalog frames.

## Spectrum capture

`FftAnalyzer` owns the demanded source's preallocated audio capture ring, FFT
buffers, preparation metadata, worker, and focused-viewer routing. Without an
active viewer the audio entrypoint returns before capture lookup or audio copy.

The worker consumes at most one 1024-sample FFT window every 33 ms. When more
than one window is available it skips to the newest complete window instead of
rendering stale spectrum history. FFT output uses `LatestAnalysis` delivery and
the dedicated native analysis outlet. Spectrum values are normalized around
the `0 dBFS` to `-90 dBFS` display range.
The presenter accepts FFT frames only from its current target instance, so a
late latest-only frame from the previous target cannot replace the new view.
The Max renderer resamples the linear FFT bins onto the same 256-point
logarithmic `20..20000 Hz` grid used by EQ curves before drawing, so both
visualizations share the same horizontal frequency geometry.
The analyzer also draws vertical reference lines at `100`, `200`, `500`,
`1000`, `2000`, `5000` and `10000 Hz` on that same logarithmic axis.
FFT values outside this display range remain unclamped in the transport and
are omitted by the renderer rather than being drawn along an analyzer edge.

## Threading

State mutation, observer notification, DSP snapshot publication, focus changes,
and analyzer configuration are control-path operations. JavaScript curve
calculation and drawing run in the Max UI scheduler. The audio callback only
copies into the bounded capture ring, claims the newest published DSP snapshot
and runs the native graph. DSP publication consumption does not depend on a
protocol callback, qelem drain, or UI activity. The callback does not call Max
UI code, allocate, block, log, or perform FFT work.

All four Max inputs remain available to Managed spectrum capture, but only the
first stereo pair enters native DSP. Reference channels are analysis inputs,
not an external DSP sidechain. Leveler, Saturator and Compressor derive private
detector signals from the current main audio at their positions in the graph.

Native processors apply the shared 10 ms parameter ramp to the continuous
scalar controls they consume. Discrete bypass, active, audibility, bank, and
filter markers update graph policy immediately. Saturator, Compressor and
Polish crossfade wet/dry over 10 ms before being skipped. Timing selector
changes also ramp the compressor's internal time constants. Detector and Equalizer filter
changes compile stable Native cascades and crossfade old and new filter outputs
over 10 ms rather than using the generic scalar ramp or interpolating raw
coefficients.
Ramp and algorithm history belong to the single audio-owned graph and are
advanced only on the audio thread.
