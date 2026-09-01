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
Committed notifications received during the active gesture update the
authoritative bindings without clearing its newer local preview. Gesture end
sends one non-coalesced final position write; the preview is cleared after that
write's response, which follows its state notifications in the lossless control
FIFO. A rejected transaction or final write also clears the preview and rebuilds
the presentation from accepted state.

The analyzer frequency response uses the prepared sample rate of the observed
source. Managed publishes this infrequent configuration as:

```text
analyzer_configuration 1 sourceInstanceId sampleRate
```

The active viewer receives the configuration when it is activated, when its
observed source changes, and when that source is prepared with a new sample
rate. Until preparation, the defined analyzer default is 48 kHz, matching the
initial DSP preparation context.

For the equalizer, Managed also publishes an atomic raw-state projection for all
banks of the observed source:

```text
analyzer_equalizer_state 1 2 sourceInstanceId bankCount equalizerActive
    (bankActive filterCount
      (filterActive filterType fixedQ parameterCount
        parameterName value × parameterCount) × filterCount) × bankCount
```

The frame carries each filter kind and its variable-length named parameters.
This projection contains no coefficients or rendered points. JavaScript remains
the only curve calculator. It caches the raw projection, calculates the response
of non-focused banks when that projection or the sample rate changes, and adds
the live focused-bank response during gestures to produce `all_banks` without
recalculating every bank on each drag frame.

The active viewer also receives `filter_catalog 1` for the selected processor.
The catalog describes each slot's filter kind, fixed Q and parameter ranges;
the UI uses it to validate the analyzer model and never derives a filter kind
from the slot index.

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
parameters. Discrete bypass, active, audibility, bank, and filter
markers are applied immediately. Ramp state belongs to the external instance
and is advanced only on the audio thread.
