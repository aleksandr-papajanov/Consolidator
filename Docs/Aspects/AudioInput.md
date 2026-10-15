# Realtime audio input ABI

Prepare is a control/setup operation: the host prepares Native buffers/runtime
and calls ConsolidatorPrepare through the Managed bridge to update analysis
configuration. It does not reset authoritative parameter state.

```text
host audio callback
→ ConsolidatorSendAudio(audioInputHandle, mainL, mainR, referenceL, referenceR, frames)
→ NativeAudioInput.ReceiveAudio → bounded capture
```

The opaque audioInputHandle resolves directly to the registered instance's
NativeAudioInput. Audio input does not query InstanceRegistry or acquire the
shared operation gate. Pointers borrow the host's prepared sample storage for
the call; retained audio is copied into bounded preallocated capture storage.
No allocations, blocking, logging, protocol delivery, FFT or optimization run
in the realtime entrypoint. Active-viewer demand controls whether capture occurs.

The main pair also feeds Native audio processing. The optional reference pair
feeds Managed sidechain analysis only. Internal device detectors are separate
main-chain taps. Handle destruction must follow the [instance lifecycle barrier](InstanceLifecycle.md).
Worker demand/analysis: [Managed StatePipeline](../../Consolidator.Managed/Docs/Aspects/StatePipeline.md).
Four-channel metadata and FFT frame: [SidechainReference](../../Consolidator.Vst3/Docs/Aspects/SidechainReference.md).