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
semantics and the all-bank sum remain unchanged. The old Max coordinate clamp
is intentionally removed from the all-bank presentation.

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

The current Equalizer layout gives the plot equal 8-unit horizontal padding and
no vertical padding; graph/list bounds are aligned below a shared 32-unit heading.
Reference, Group and All Banks independently gate their cached Paths. The new
sidechain reference has its own spectrum cache; filter edits do not rebuild it.
See [Vst3SidechainReference.md](Vst3SidechainReference.md) for capture and delivery.
These subsequent changes are source-reviewed only and have not been built or
visually verified; the build records below refer to earlier changes.

Source review covers target/mode changes, empty/group spectra, sample-rate
changes, response revision invalidation, logical resizing, marker hit regions and
message-thread ownership. No DSP tests, automated UI tests or timing benchmarks
were run for this presentation change. Live checks remain for spectra, narrow
peaks, extreme EQ curves, mode changes, dragging and zoom.

Release x64 build and installation completed on 2026-10-14 with exit code 0
and no warnings in the build output. The installed module hash matched the
artifact. Existing Managed runtime was reused. Live visual verification and
frame-time measurements remain pending.

The synchronized focused/all-bank preview fix was also built and installed in
Release x64 on 2026-10-14, with exit code 0, no build warnings and matching
artifact/installed module hashes. Source review covered delayed snapshots,
focused bank 0..6, bypass, detector contexts and preview completion/rejection.
Its visual drag behavior still requires a Live pass; no tests were run.

The all-bank coordinate-clamp removal compiled in Release x64 on 2026-10-14
with no build warnings. Installation initially failed with permission denied
while Ableton Live was open. Source inspection confirmed that Native EQ cascades
do not impose the analyzer's +/-24 dB plot-range limit. Audio algorithms were
not changed or tested; the visual correction still requires a Live pass.
After the user closed Live, installation succeeded with exit code 0. The
installed module hash matched the build artifact.

The filter-button removal, Ctrl-click bypass and visible bypassed handles built
and installed in Release x64 on 2026-10-14 with exit code 0, no warnings and
matching module hashes. Source review covered active/bypassed marker hit testing,
Ctrl-click before gesture begin, double-click reset, detector modes and cache
invalidation after plot expansion. No tests or Live gesture checks were run.

The bypassed gray filter shapes, continuous analyzer/Q gestures, removal of Rough
dots/global precision state, Shift-only dial snapping and larger track indicators
were built and installed in Release x64 on 2026-10-14 with the frontend refactor.
Build/install completed with exit code 0, no warnings and matching module hashes.
Existing Managed runtime was reused. No tests or Live gesture checks were run.
