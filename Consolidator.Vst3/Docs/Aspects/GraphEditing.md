# Graph editing and response preview

## Analyzer response calculation

`AnalyzerCurveCalculator` evaluates presentation responses: gain, tilt, low/high shelf
and bell responses, with 1024 logarithmic frequency points from 20 to 20000 Hz.
Coefficients are calculated once per filter. Bypassed filters retain their
stored response shape and draw in the same gray as their markers; the combined response includes only active
filters when the bank is active. Plot coordinates use the existing -24/+24 dB
scale and remain unclamped for renderer clipping.

This is frontend presentation code, not a replacement for Native DSP. Its
caller must provide a prepared positive sample rate and valid authoritative
bindings, including positive frequency and Q. It contains no state defaults,
protocol requests or audio-thread work.

`AnalyzerControl` is connected to accepted target snapshots and state changes
for Equalizer and the Gain Staging, Saturator and Compression detectors. It retains
only relevant presentation bindings. The shared Native filter catalog provides
filter definitions; fixed Q is presentation metadata, while editable values
and physical/effective ranges come from Managed. The view is cleared during
target transitions and rejects FFT frames from another target instance.

Main, group and optional sidechain reference spectra use Managed `fft` version 2
(1024 samples, 513 bins per spectrum); wire shape has one source in
[SpectrumProtocol](../../../Consolidator.Managed/Docs/Aspects/SpectrumProtocol.md). Smoothing is 0.5 for attack and 0.16
for release. Spectra use a separate logarithmic grid based on logical plot
width, retaining the loudest interpolated/bin value within each column.
Finite values outside the visible vertical range are clipped to the plot instead
of breaking the curve. These intentional rendering choices and
the independent geometry caches are described in [Vst3AnalyzerRendering.md](AnalyzerRendering.md).
The peer spectrum uses the bank's group color regardless of edit
scope. Reference frequency geometry, filter colors and plot margins follow the shared presentation contracts.

The processor publishes its prepared sample rate through an atomic control
value; its existing message-thread timer emits the local editor event
`audio_configuration_changed` only when that value changes. This is not a
Managed protocol extension. No synthetic 48 kHz default is introduced. Before
audio preparation the analyzer remains blank; release/rate changes discard
old spectra. The FFT protocol carries no remote source sample rate,
so remote focus uses the current plugin's host sample rate, as the existing
frontend does. This depends on instances sharing the host audio configuration
and remains a host-validation item.

Filter handles support frequency/gain dragging, Alt-drag Q and double-click
reset. Frequency, gain and Q gestures are continuous within effective limits.
The analyzer has no Rough/Precise state, modifier-based snapping or Rough-grid
dots. Frequency uses the logarithmic plot axis; Alt-drag Q uses a linear
increment of 0.02 Q per logical vertical pixel. Fixed frequency reference lines
remain. Temporary Shift snapping is exclusive to native dials;
see [Vst3Controls.md](Controls.md). Ctrl-click toggles filter bypass without
starting a drag/history gesture. Bypassed filter handles remain visible as dim
outlines and keep the same hit region, dragging and double-click reset behavior.
Separate numbered filter buttons are removed.

`DialGestureSession` now retains a batch of typed `StateWriteEntry` values.
Scalar dials use one entry; analyzer frequency/gain changes use one existing
Managed `write` command containing both entries. Q changes join that gesture's
latest batch. The same begin acknowledgement, 33 ms coalescing, final
transaction-zero acknowledgement, history end and editor-close behavior serve
both controls. There is no second gesture transport. Group floats retain
`delta` mode and authoritative effective limits. Local preview survives
intermediate notifications and is cleared after the final acknowledgement or
rejection; filter-row gestures also update the displayed curve immediately.
Filter reset invokes the existing Managed reset command, which owns history.

The Equalizer also requests the existing `observe_banks` snapshot and displays
the combined response of all seven banks in the original overlay color. One
request may be outstanding; further EQ notifications mark it dirty and trigger
one fresh request after the pending reply, rather than issuing a request per
entry in a position write. Replies must match the current request, target and
context; changing target invalidates them. All seven distinct banks are parsed
before replacing the presentation cache. Focused and all-bank curves use the
same filter reader and response calculator. The overlay combines accepted
responses of the other six banks with the focused bank's current accepted/preview
response, so both curves use the same gesture position. The focused bank index
comes from the authoritative target binding; its contribution in a delayed
bank snapshot is excluded. Its
summed coordinates remain unclamped, with strokes clipped by the plot rectangle
instead of flattening out-of-range peaks. The overlay is absent when every bank
is bypassed.
