# DSP state and analyzer presentation

## Ownership

The Managed state tree is the only source of truth for user parameters. Value
observers project committed changes into the per-instance `DspRuntimeState`, and
the resulting fixed-layout `DspSnapshot` is published to Native. The audio
callback reads only that published snapshot.

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
On rejection the preview is cleared and the presentation is rebuilt from the
accepted state. A committed state notification likewise clears the preview and
rebuilds the curve.

The analyzer frequency response uses the prepared sample rate of the observed
source. Managed publishes this infrequent configuration as:

```text
analyzer_configuration 1 sourceInstanceId sampleRate
```

The active viewer receives the configuration when it is activated, when its
observed source changes, and when that source is prepared with a new sample
rate. Until preparation, the defined analyzer default is 48 kHz, matching the
initial DSP preparation context.

The UI renders only the focused bank response. There is no partial or cached
`all_banks` presentation assembled from banks that the client has not loaded.

## Spectrum capture

`FftAnalyzer` owns the demanded source's preallocated audio capture ring, FFT
buffers, preparation metadata, worker, and focused-viewer routing. Without an
active viewer the audio entrypoint returns before capture lookup or audio copy.

The worker consumes at most one 1024-sample FFT window every 33 ms. When more
than one window is available it skips to the newest complete window instead of
rendering stale spectrum history. FFT output uses `LatestAnalysis` delivery and
the dedicated native analysis outlet.

## Threading

State mutation, observer notification, DSP snapshot publication, focus changes,
and analyzer configuration are control-path operations. JavaScript curve
calculation and drawing run in the Max UI scheduler. The audio callback only
copies into the bounded capture ring and reads the published native DSP state;
it does not call Max UI code, allocate, block, log, or perform FFT work.

The native audio path applies a 10 ms linear ramp to continuous runtime snapshot
parameters. Discrete bypass, solo, active, listen, audibility, bank, and filter
markers are applied immediately. Ramp state belongs to the external instance
and is advanced only on the audio thread.
