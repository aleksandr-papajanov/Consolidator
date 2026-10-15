# VST3 sidechain reference spectrum

The VST3 declares stereo Input/Output and an optional stereo Sidechain input,
disabled by default. Ableton routes the reference signal to this bus. Both float
and double processBlock pass dry main and reference pairs through the existing
ManagedBridge::SendAudio call. Float uses four preallocated double scratch arrays;
double uses bus pointers directly. Chunk offsets/capacity match for both pairs.
Only the main pair enters Native DSP; the reference input remains unmodified.

Capture ring, connection metadata and worker processing are defined in
[Managed SpectrumAnalysis](../../../Consolidator.Managed/Docs/Aspects/SpectrumAnalysis.md).

## FFT transport

The wire frame is defined in [SpectrumProtocol](../../../Consolidator.Managed/Docs/Aspects/SpectrumProtocol.md).
ProtocolClient filters command-completion selectors before validating command
headers. AnalyzerClient owns FFT version and payload validation; editor routing
does not treat the FFT notification as a command response.
Both optional flags explicitly determine payload length/order. No old frame decoder
or parallel reference event remains. The normal Native callback copies it into
PluginProcessor's latest FFT slot; AnalyzerClient validates version, source,
size, shape and finite values before delivery to AnalyzerViewModel. Empty reference
payload clears the reference presentation after Sidechain disconnection.

## Presentation

AnalyzerViewModel smooths/resamples all spectra through the same pipeline, with
0.5/0.16 smoothing, logarithmic peak-preserving columns and Nyquist bounds.
The Reference, Group and All Banks switches gate drawing only, remain local to
the editor and initially show their overlays. Hidden data continues updating.
The reference Path uses the Input marker color and normal plot clipping.
There is no frontend FFT worker, JUCE FFT dependency or host-local reference event.

Host-validation scenarios: [Validation](Validation.md).
