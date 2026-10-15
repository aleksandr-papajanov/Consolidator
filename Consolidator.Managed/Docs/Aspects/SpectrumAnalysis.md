# Spectrum capture and worker analysis

## Spectrum capture

`FftAnalyzer` owns preallocated audio capture rings for the demanded focused
source and, when that bank is grouped, every instance represented in the same
bank group. It also owns FFT buffers, preparation metadata, the worker and
focused-viewer routing. Without an active viewer the audio entrypoint returns
before capture lookup or audio copy.

The worker consumes at most one 1024-sample FFT window per demanded instance
every 33 ms. When more than one window is available it skips to the newest
complete window instead of rendering stale spectrum history. It transforms the
focused source independently, sums the time-domain windows of the focused source
and every other group instance, and transforms that sum as the group spectrum.
An ungrouped bank has no group spectrum. FFT output uses `LatestAnalysis`
delivery and the native latest FFT slot. Spectrum values are
normalized around the `0 dBFS` to `-90 dBFS` display range.
The presenter accepts FFT frames only from its current target instance, so a
late latest-only frame from the previous target cannot replace the new view.
It renders the source spectrum with the existing analyzer color and the summed
peer spectrum with the focused bank group's color, independently of whether the
edit scope is currently local or group.
The focused source's optional Sidechain pair uses the same capture and worker FFT pipeline. The version-2 frame carries main/group/reference spectra; VST3 resamples and clips them for display. See [SidechainReference](../../../Consolidator.Vst3/Docs/Aspects/SidechainReference.md) for capture metadata and transport, and [AnalyzerRendering](../../../Consolidator.Vst3/Docs/Aspects/AnalyzerRendering.md) for frequency geometry.


Demand/focus changes run on the control path. Audio entrypoints only capture into
bounded prepared storage; FFT work runs on the Managed worker. The main stereo
pair feeds both Native DSP and analysis; Sidechain feeds reference analysis only.
Frame layout: [SpectrumProtocol](SpectrumProtocol.md).

## Reference capture

NativeApi.Audio forwards the reference pointers through NativeAudioInput and
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
future application operations to consume it within Managed.

Capture follows the existing selected-source/group demand. The reference belongs
to the focused source instance, not necessarily the plugin hosting the editor.
Selecting another track selects its main and Sidechain audio together. Removing
an instance/demand or changing preparation uses the existing capture lifecycle.

