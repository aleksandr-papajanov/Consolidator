# VST3 sidechain reference spectrum

The VST3 declares stereo Input/Output and an optional stereo Sidechain input,
disabled by default. Ableton routes the reference signal to this bus. Both float
and double processBlock pass dry main and reference pairs through the existing
ManagedBridge::SendAudio call. Float uses four preallocated double scratch arrays;
double uses bus pointers directly. Chunk offsets/capacity match for both pairs.
Only the main pair enters Native DSP; the reference input remains unmodified.

## Managed ownership

The existing four-pointer ConsolidatorSendAudio ABI is unchanged. NativeApi.Audio
now forwards the reference pointers through NativeAudioInput and
IInstanceAudioInputService into FftAnalyzer. AudioCapture stores all four channels
in one bounded preallocated SPSC ring with shared read/write positions. Both
stereo pairs therefore use the same sample clock/window and overflow policy.
Absent reference pointers store zeros. Connection metadata is stored alongside
each audio frame in the ring; only the worker derives the published connection
flag from its actual latest window. An unavailable window publishes no reference.
Realtime input does not allocate, lock, transform audio or retain ABI pointers.

The existing Managed worker extracts the latest 1024-sample Hann window for main
and reference. It uses SpectrumProcessor for both spectra, including its existing
normalization. The optional group spectrum remains the sum of main peer signals;
reference audio is not mixed into the group or device detector paths.
FftAnalyzer owns the reference window/spectrum alongside main analysis, allowing
future application operations to consume it within Managed. No approximator
operation or new persistence behavior is added by this change.

Capture follows the existing selected-source/group demand. The reference belongs
to the focused source instance, not necessarily the plugin hosting the editor.
Selecting another track selects its main and Sidechain audio together. Removing
an instance/demand or changing preparation uses the existing capture lifecycle.

## FFT transport

SpectrumPublisher sends one latest-only fft notification containing every spectrum
for the same source:

```text
fft 2 sourceInstanceId 1024 hasGroup hasReference
    sourceSpectrum[513] [groupSpectrum[513]] [referenceSpectrum[513]]
```

Version 2 applies to this notification; command protocol headers stay unchanged.
ProtocolClient first filters command-completion selectors before checking their
version-1 header. Editor routing keeps version-1 validation for other notifications;
fft version/shape belongs to AnalyzerClient. Previously the command client checked
every incoming frame against version 1, rejected fft version 2 and failed the
editor session, leaving Inactive visible despite window focus. This correction
is source-reviewed only and has not been built or verified in Live.
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

## Verification

Source review covered the full four-pointer path, float/double chunking, capture
ownership, reference absence, worker FFT, group isolation, latest delivery, strict
frame decoding and UI clearing. Existing call sites and protocol expectations in
fixtures were updated to the new contract. No build, tests, audio experiments or
Live routing checks were run. A matching rebuilt Managed runtime and VST3 are
required for installation; the previously installed binaries are unchanged.
