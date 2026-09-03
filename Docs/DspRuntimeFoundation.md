# DSP Runtime Foundation

## Scope

This scope defines the native DSP execution model and the first input-level
audio algorithm. Managed owns the authoritative state tree and publishes a fixed
`DspSnapshot`. Native consumes that snapshot and owns the realtime execution
state for one external instance.

Native files are separated by ownership scope:

- `Consolidator.Native/External/Dsp` uses the `consolidator::max::dsp`
  namespace for realtime processors, graph orchestration, smoothing, and the
  shared DSP state layout.
- `Consolidator.Native/External/Transport` uses the
  `consolidator::max::transport` namespace for atoms, Managed bridge, ABI
  declarations, and persistence transport helpers.
- `ConsolidatorExternal` remains in `consolidator::max` as the Max-facing
  orchestration boundary that composes both scopes.

```text
Managed StateValue
    -> DspRuntimeState
    -> DspSnapshot
    -> DspStateExchange
    -> DspParameterSmoother
    -> DspRuntimeGraph
    -> main audio output
```

`DspSnapshot` is a transport contract. It does not contain native runtime
state, compiled coefficients, audio buffers, or pointers. Native runtime state
is never sent back to Managed.

## Runtime graph

Each external owns one fixed graph with this order:

```text
InputProcessor
  -> EqualizerDetectorProcessor
  -> LevelerProcessor
  -> SaturatorProcessor
    -> CompressorProcessor
    -> EqualizerProcessor
    -> CompressorParallelStage
    -> PolishProcessor
    -> PolishParallelStage
  -> OutputProcessor
  -> LimiterProcessor
```

Input and Output are processors rather than special gain operations. This
keeps their lifecycle, parameter binding, bypass handling, and future runtime
state consistent with the other devices.

The graph uses a static set of processor members instead of a heap-owned
processor list. Processor dispatch is therefore explicit and the audio path
has no registry lookup or allocation. `DspProcessor` is the lifecycle and
processing contract; `InputProcessor` applies the input level as dB gain and
the remaining processors currently implement identity behavior.

## Execution policy

`DspExecutionPolicy` is derived when a new snapshot is accepted. The graph
consults it before invoking each processor, so inactive devices are skipped as
a whole instead of repeating global activity checks inside algorithms.

The policy defines these graph-wide decisions:

- `audible == false` clears both main output channels and exits before device
  processing;
- `instanceBypass` disables the graph while preserving input ownership;
- `DspChannelMode` selects stereo, left-only, or right-only lanes, allowing
  mono work to avoid a second channel pass;
- `widthProcessingRequired` identifies whether the input width control needs a
  stereo operation; a neutral width does not force every later processor to
  process both lanes;
- `inputLeveler` gates the separate `LevelerProcessor`;
- `saturatorSplit` selects the bounded frequency-band dispatch contract;
- `compressorParallel` gates the reusable parallel branch and equal mix;
- `outputLimiter` gates the separate `LimiterProcessor`.

The policy is derived orchestration state, not a second parameter source.

## Lifecycle

`Prepare(sampleRate, maximumFrameCount)` runs on the control/setup path. It
stores the processing context and allocates any bounded scratch storage before
audio starts. It does not reset Managed state or publish a new snapshot.
When the host does not provide a vector size, the graph uses the documented
bounded capacity of 4096 frames; blocks larger than that are rejected rather
than causing an audio-thread allocation.

`SetParameters(snapshot)` is called only on the control/setup path. The
external prepares one of three graph slots, publishes it with an atomic index,
and the audio callback acquires that slot with a reader reservation. A graph
slot is not reused while an audio callback still owns it, so processors may
compile coefficients or update derived state during `SetParameters` without
mutating the graph currently used by audio. `Reset()` is reserved for an
explicit audio lifecycle reset.

`ProcessBlock()` is realtime code. It accepts caller-owned main buffers and
non-owning sidechain views, performs no allocation, locking, logging, or Max
calls, and returns without touching audio if its inputs are invalid or exceed
the prepared capacity. If a graph slot cannot be acquired during a concurrent
publication, the external leaves its already-copied output as passthrough for
that block.

## Channel and frequency scope

`DspProcessingPlan` is a control-path selection shared by every processor in
the graph. `DspChannelMode` supports independent left-channel processing,
independent right-channel processing, and linked stereo processing. The plan
does not duplicate audio buffers; processors select the channel lanes they need
from the caller-owned main buffer.

`DspFrequencyBandSet` stores a bounded list of frequency ranges. A processor
may apply its future algorithm to the selected ranges while leaving other
ranges untouched. Range selection is separate from filter definitions in
`DspSnapshot`: snapshot filters describe device parameters, while the plan
describes where a processor is allowed to act. Ranges are validated and copied
before audio processing and are bounded to the prepared Nyquist frequency, so
the audio callback never allocates or parses range data.

The foundation deliberately does not perform channel splitting, crossover,
FFT, or band-limited processing yet. Those algorithms must consume the plan
inside a concrete processor and preserve the same no-allocation realtime
contract.

## Sidechain and parallel processing

Every processor receives the main stereo buffer and a separate read-only
sidechain view. The external reference inputs are connected as the external
sidechain and are intentionally not copied into the main output.

`DspParallelStage` owns preallocated branch buffers and provides the branch
lifecycle and mix boundary needed for parallel devices. The graph owns
separate instances for compressor parallel processing and the two-branch
Polish topology. Compressor parallel mode is disabled unless its snapshot flag
is enabled; Polish branches are enabled with the Polish device. Until concrete
algorithms are supplied, branch processing and mixing remain identity
operations. Enabling parallel work must never allocate on the audio thread.

The processing frame carries two sidechain views: the external sidechain input
and the detector sidechain route. `EqualizerDetectorProcessor` is the explicit
producer stage for detector material; the leveler and compressor consume that
route while applying their processing to the main signal. Until detector
processing is implemented, the graph initializes the detector route from the
external sidechain as a pass-through. The sidechain is listened to by a device
while its processing is applied to the main signal.

`DspProcessingPrimitives.h` provides reusable boundaries for future devices:
`SelectChannels` selects allowed lanes, `DspFrequencyBandRunner` iterates
selected ranges without audio-thread allocation, and `DspParallelMixer`
defines a 50/50 wet/dry mix. The leveler, compressor, and Polish topology are
represented by separate processor slots and routing boundaries. Detector
filtering, crossovers, dual Polish branches, limiting, and all other concrete
audio operations remain outside this foundation.

## Parameter policy

The existing native smoother owns the transition from published parameters to
continuous runtime values. Discrete flags and processor markers remain
immediate. Filter coefficient smoothing and derived-state compilation belong to
future processor implementations and must not be added to Managed state.

`InputLevel` uses the Managed range `-36..36 dB`. Native converts it to linear
gain during `SetParameters`; `InputProcessor::Process` only multiplies the
selected main lanes, so logarithmic conversion remains outside the audio loop.

## Testing contract

The native foundation tests verify:

- preparation retains sample rate and maximum frame count;
- the complete identity graph preserves main audio;
- `InputLevel` applies the expected dB gain to stereo audio;
- input gain bypass preserves the original signal;
- sidechain buffers are accepted without becoming output;
- an oversized block is rejected without partial processing;
- the existing snapshot exchange and parameter smoother contracts remain
  unchanged.

Additional audio algorithms, coefficient correctness, latency, denormal
handling, and Max host integration are outside this scope.
