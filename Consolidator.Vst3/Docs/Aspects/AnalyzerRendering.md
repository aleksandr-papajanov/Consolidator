# VST3 analyzer rendering

## Ownership and invalidation

Managed remains the FFT producer. The existing 1024-sample, 513-bin wire frame,
latest-only delivery and 30 Hz message-thread drain are unchanged. No analysis,
protocol or Native DSP behavior is moved into the frontend.

`AnalyzerViewModel` owns smoothed bins and normalized presentation values.
`AnalyzerControl` owns marker interaction and its `AnalyzerRenderer`.
The renderer owns reusable JUCE Paths for frequency reference lines, response curves,
spectra, plot clip and Gain marker. All are accessed on the message thread.

An FFT frame updates only spectrum values/paths and invalidates the plot area.
It does not recalculate responses. There are no filter-button child controls;
Ctrl-click on a visible handle toggles bypass, including when already bypassed.
Bypassed handles use dim outlines and retain dragging/reset hit regions. Ordinary
double-click resets a filter; the double-click reset takes priority over Ctrl.
Bypassed filter response paths retain their parameter-defined shape in the same
gray as the handles. They remain excluded from focused and all-bank sums.
Response/all-bank rebuilds advance a presentation revision; the renderer rebuilds
response paths only when that revision or plot bounds change. Mode/target clears
invalidate response geometry and hide discarded spectra. Color, selection,
scope changes use existing paths. There is no Rough/Precise mode or dot grid;
frequency/gain/Q editing is continuous and ignores Shift snapping.
Interior vertices of exactly horizontal runs are omitted; flat filter responses
use a single segment without changing their shape or approximating curved runs.

The all-bank overlay uses a cached, unclamped sum of the six non-focused banks
from accepted snapshots, plus the focused bank's current response. The binding
supplies the focused bank index. Every accepted/preview response rebuild also
composes the overlay before either Path is published. The final sum remains
unclamped, like the focused response; only the renderer's plot clip limits its
visibility. Clamping coordinates formerly flattened peaks at the plot boundary
and falsely suggested an EQ gain limit. Native EQ has no such plot-range limit.
Late bank snapshots refresh only the other-bank contribution, so they cannot
overwrite the focused preview. Finishing or rejecting a gesture recomposes both
curves from the accepted focused values. Detector contexts have no all-bank curve;
target clears discard the cached bank sum. The overlay remains absent when all
banks are bypassed.

This corrects the former data-flow mismatch: local drag immediately rebuilt the
focused response, while the overlay waited for a separate `observe_banks` reply
after the coalesced Managed write. The two curves showed different parameter
positions even though the renderer drew them in one paint call.

`paint()` draws prepared geometry through JUCE Graphics. The analyzer is opaque
because it fills its background. It does not use a whole-component image buffer
or a separate OpenGL context. Uniform editor zoom keeps logical plot geometry;
the existing component transform scales it.

## Sampling and clipping

Responses use 1024 logarithmic points over 20..20000 Hz instead of the former
256. Frequency values are prepared once; each response calculation shares its
trigonometric basis across all filters. Coefficients, fixed Q, active/bypass
semantics and the all-bank sum remain unchanged. Display paths are not clamped
point by point; their strokes are clipped to the plot rectangle.

Spectra have approximately one point per logical plot column, independent of
the response grid. Column frequency intervals are prepared only when plot
resolution changes. After the existing attack/release smoothing (0.5/0.16),
each column retains the loudest value of its interpolated endpoints and enclosed
FFT bins. This prevents narrow bin peaks being skipped by point sampling.
Columns above Nyquist have no geometry; the last bin is never extrapolated.
Changing audio sample rate clears the previous bins before new frames arrive.

Finite response/spectrum coordinates remain unclamped in their Paths. The
renderer clips their strokes to the exact plot rectangle, preserving boundary
intersections instead of deleting out-of-range points. Non-finite spectrum
coordinates delimit unavailable regions. Handles are drawn after restoring
the clip, retaining the Gain arrow at the left plot edge.

## Validation

Current host scenarios and diagnostic procedure: [Validation](Validation.md).
Build/install success does not establish visual or interaction correctness.
