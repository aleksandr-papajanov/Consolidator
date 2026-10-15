# Spectrum frame protocol

## FFT payload

SpectrumPublisher emits one latest-analysis notification for the focused source:

```text
fft 2 sourceInstanceId 1024 hasGroupSpectrum hasReferenceSpectrum
    source[513] [group[513]] [reference[513]]
```

The new header version is specific to fft; command headers retain their current
version. Reference is the focused source's optional Sidechain pair captured and
transformed in Managed, alongside main audio. Group includes only main peer
signals. Flags determine exact optional payload order/length. Absence of the
reference pair clears its payload, preventing stale reference presentation.
VST3 validates version, source context, FFT size, exact shape and finite bins.
There are no compatibility decoders or separate reference events.
