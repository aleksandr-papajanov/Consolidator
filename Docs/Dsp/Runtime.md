# DSP Runtime Foundation

## Scope

This scope defines the native DSP execution model and the first input-level
audio algorithm. Managed owns the authoritative state tree and publishes a fixed
`DspSnapshot`. Native consumes that snapshot and owns the realtime execution
state for one external instance.

Native files are separated by ownership scope:

- `Consolidator.Native/External/Dsp` uses the `consolidator::max::dsp`
  namespace for realtime processors, graph orchestration, smoothing, and the
  shared DSP state layout. Each processor has its own `.h` contract and `.cpp`
  translation unit. Shared primitives and hot-path choices are described in
  [`DspImplementation.md`](Implementation.md).
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
    -> DspRuntime
    -> DspRuntimeGraph
    -> main audio output
```

`DspSnapshot` is a one-way parameter transport contract. It does not contain
native runtime state, compiled coefficients, audio buffers, or pointers.
Selected display-only runtime values return through the separate bounded
latest-only contract described in [`DspTelemetry.md`](Telemetry.md).

## Runtime graph

Each external owns one fixed graph with this order:

```text
InputProcessor
  -> Width
  -> [internal detector EQ -> Leveler-controlled Input Gain]
SaturatorProcessor
  -> internal detector EQ
  -> saturation (drive, curve, optional split bands)
  -> saturator output gain
CompressorParallelStage
  -> dry path
  -> wet path: internal detector EQ
       -> compression
       -> shared transient shaper (signed attack/sustain, own HP20 detector)
       -> compressor output gain -> permanent transient output clamp
  -> 50/50 dry/wet when parallel is enabled
EqualizerProcessor
PolishProcessor
  -> Thick: add level-calibrated bass harmonics with upward compression
  -> Air: broad/top shelves plus dynamically controlled upper harmonics
OutputProcessor
  -> output level
  -> limiter
Final linked 300 ms RMS meter
```

Input and Output are processors rather than special gain operations. This
keeps their lifecycle, parameter binding, bypass handling, and future runtime
state consistent with the other devices.

The graph uses a static set of processor members instead of a heap-owned
processor list. Processor dispatch is therefore explicit and the audio path
has no registry lookup or allocation. `DspProcessor` is the lifecycle and
processing contract. Width and the four explicit level/output gain stages are
implemented. Leveler is the first dynamics algorithm. Detector equalization and
the main Equalizer are implemented with Native biquad cascades. Saturation,
transient shaping, adaptive compression, Polish Thick/Air and sample-peak limiting
have native algorithms. Adaptive compression is specified separately in
[`AdaptiveCompression.md`](CompressionProcessor.md).

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
  stereo operation; Width uses the normalized `0..1` state value and `1` is
  neutral, so a neutral width does not force every later processor to process
  both lanes;
- `inputLeveler` gates the separate `LevelerProcessor`;
- `saturatorSplit` selects the bounded frequency-band dispatch contract;
- `compressorParallel` selects a normalized 50/50 dry/wet mix;
- `outputLimiter` gates the separate `LimiterProcessor`.

The policy is derived orchestration state, not a second parameter source.

## Lifecycle

`Prepare(sampleRate, maximumFrameCount)` runs on the control/setup path. It
stores the processing context and allocates any bounded scratch storage before
audio starts. It does not reset Managed state or publish a new snapshot.
When the host does not provide a vector size, the graph uses the documented
bounded capacity of 4096 frames; blocks larger than that are rejected rather
than causing an audio-thread allocation.

Each external owns one `DspRuntime` and one audio-owned `DspRuntimeGraph`.
At the beginning of a block, `DspRuntime` claims and copies the newest
`DspSnapshot` directly from `DspStateExchange`, independently of Managed
protocol output or UI activity, and applies it to that graph. Processor history,
envelopes and other realtime state therefore remain continuous across parameter
publications. `SetParameters(snapshot)` is realtime-safe: it may update scalar
targets or select precompiled immutable data, but may not allocate, lock, log,
or rebuild unbounded state. `Reset()` runs on an explicit audio lifecycle reset
or once after a device's wet-to-dry transition finishes, so stale detector and
filter history is not resumed after inactivity.

`ProcessBlock()` is realtime code. It accepts caller-owned main buffers;
detector views are created only inside device processors from preallocated
storage. Processing performs no allocation, locking, logging, or Max calls and
returns without touching audio if its inputs are invalid or exceed the prepared
capacity.

After graph processing, `DspRuntime` calculates one bounded block mean-square
value across the final output channels and advances a 300 ms exponential energy
average. Its dBFS result is written to the shared telemetry seqlock. This meter
does not alter audio and performs no allocation, locking, callback, or protocol
formatting on the audio thread.

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

Arbitrary frequency-plan selection is not yet implemented by the algorithms.
Saturator Split uses its own fixed complementary crossovers. DSP performs no FFT.

## Sidechain and parallel processing

Detector sidechains are internal device signals. The Max reference inputs do
not enter either Managed analysis or the native DSP graph. A device detector
copies the current main signal at that exact point in the chain into preallocated
buffers, applies its detector EQ there, and exposes the result as a read-only
control signal. The listening algorithm modifies only the main path; detector EQ
audio is never mixed into the output.

`DspParallelStage` owns preallocated branch buffers and a 10 ms wet ramp.
Saturator, Compressor and Polish each use a stage to crossfade activity and
device bypass. A processor is skipped only after its wet ramp reaches zero.
Compressor's target is 0.5 in Parallel and 1 otherwise; Output gain runs on the
wet branch before interpolation. Identical unity branches therefore preserve
unity in both modes.
At settled 100% wet, the processor runs directly on main audio without branch
copying or identity interpolation. A local processing frame isolates detector
route changes; transitioning and parallel stages retain their branch storage.

Polish applies Thick followed by Air. Each processor adds its own enhancement
to dry audio, so there is no averaging that halves either effect.
Polish itself needs no copied branch buffers; only its graph activity stage
owns preallocated dry/wet storage.

The processing frame carries the current internal detector route. Input,
Saturator and Compressor each own a `DetectorProcessor` with preallocated output
buffers and their corresponding pair of detector-filter snapshot slots. Input
captures after Width and before the Leveler-controlled Input Gain, Saturator captures its device input, and
Compressor captures the wet-branch input for compression. Its shared transient
shaper reads the post-compression main audio through its own HP20 detector. Each EQ detector
applies its LowShelf and Bell cascade only to those owned buffers; the main audio
remains separate.

`DspProcessingPrimitives.h` provides a non-owning selected-channel view with
peak, mean-square and gain application helpers. Algorithm-specific detector
linking remains explicit. Leveler, compressor and Polish retain separate
processor slots and routing boundaries.

## Parameter policy

`DspParameterRamp` is the reusable ten-millisecond linear smoothing primitive.
Each processor owns one instance for every continuous scalar it consumes and
sets only its target when a snapshot is accepted. Implemented algorithms read
`NextValue()` per sample; identity algorithm slots advance the same state in
O(1) per block so later implementations inherit continuous runtime state.
Unchanged targets do not restart a ramp, and retargeting continues from the
current audio-owned value.

Width, Leveler target, Saturator drive and curve, Compressor attack, sustain and
amount, Polish Thick and Air, and Limiter target use this common mechanism.
Discrete flags, enum values and processor markers update graph policy
immediately. Filter frequency, gain, Q and filter/bank activity are deliberately
excluded from scalar ramps: detector and Equalizer processors compile stable
cascades and crossfade the old and new outputs over ten milliseconds instead of
interpolating raw coefficients.

`InputLevel` uses the Managed range `-36..36 dB`. `LevelerProcessor` owns its
manual dB ramp so it can transfer the current automated value without a jump;
after the ten-millisecond transition it calculates the static linear gain only
once per block. Saturator output and final Output level use
`DspGainRamp`, which delegates sample progression to `DspParameterRamp`. Compressor
output uses the shared transient processor's linear-gain DspParameterRamp before
its permanent final clamp. A
transition back to neutral continues until unity is reached even after Managed
clears the input activity marker. Saturator, Compressor and Polish continue
processing through their wet-to-dry transition before resetting runtime history.


## Алгоритмы

Каждый процессор описан отдельно в [индексе DSP](README.md).
