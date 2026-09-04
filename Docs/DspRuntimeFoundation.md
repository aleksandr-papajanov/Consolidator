# DSP Runtime Foundation

## Scope

This scope defines the native DSP execution model and the first input-level
audio algorithm. Managed owns the authoritative state tree and publishes a fixed
`DspSnapshot`. Native consumes that snapshot and owns the realtime execution
state for one external instance.

Native files are separated by ownership scope:

- `Consolidator.Native/External/Dsp` uses the `consolidator::max::dsp`
  namespace for realtime processors, graph orchestration, smoothing, and the
  shared DSP state layout. Each processor implementation has its own `.cpp`
  translation unit; `DspProcessors.h` contains their shared public contracts.
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
latest-only contract described in [`DspTelemetry.md`](DspTelemetry.md).

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
       -> transient shaper (attack, sustain)
       -> compression
       -> compressor output gain
  -> dry + wet when parallel is enabled
EqualizerProcessor
PolishProcessor
  -> Thick branch
  -> Air branch
  -> equal 50/50 branch mix
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
transient shaping, compression, Polish coloration and limiting retain identity
behavior inside their final routing positions.

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
- `compressorParallel` preserves dry audio and adds the processed wet branch;
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
or rebuild unbounded state. `Reset()` is reserved for an explicit audio
lifecycle reset.

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

The foundation deliberately does not perform channel splitting, crossover,
FFT, or band-limited processing yet. Those algorithms must consume the plan
inside a concrete processor and preserve the same no-allocation realtime
contract.

## Sidechain and parallel processing

Detector sidechains are internal device signals. The Max reference inputs are
used only by Managed analysis and never enter the native DSP graph. A device
detector copies the current main signal at that exact point in the chain into
preallocated buffers, applies its detector EQ there, and exposes the result as a
read-only control signal. The listening algorithm modifies only the main path;
detector EQ audio is never mixed into the output.

`DspParallelStage` owns preallocated compressor branch buffers. It preserves the
dry signal, runs the complete detector/transient/compression/output chain on the
wet branch, then adds wet to dry. Because compressor output gain runs before the
sum, it controls how much compressed signal is added. Without parallel mode the
same complete compressor chain processes main audio in place.

Polish owns two separate preallocated copies of its input. Thick and Air process
their own branches and are mixed 50/50. Equal averaging is the current explicit
unity-safe policy: two identity branches reproduce the input instead of doubling
it. Enabling either parallel topology must never allocate on the audio thread.

The processing frame carries the current internal detector route. Input,
Saturator and Compressor each own a `DetectorProcessor` with preallocated output
buffers and their corresponding pair of detector-filter snapshot slots. Input
captures after Width and before the Leveler-controlled Input Gain, Saturator captures its device input, and
Compressor captures the wet-branch input before transient shaping. Each detector
applies its LowShelf and Bell cascade only to those owned buffers; the main audio
remains separate.

`DspProcessingPrimitives.h` provides reusable boundaries for future devices:
`SelectChannels` selects allowed lanes, `DspFrequencyBandRunner` iterates
selected ranges without audio-thread allocation, and `DspParallelMixer`
defines the compressor wet-into-dry sum. The leveler, compressor, and Polish
topology is represented by separate processor slots and routing boundaries.
Crossovers, coloration, compression, and limiting remain identity algorithm
slots until their transfer functions are implemented.

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
once per block. Saturator output, Compressor output and final Output level use
`DspGainRamp`, which delegates sample progression to `DspParameterRamp`. A
transition back to neutral continues until unity is reached even after Managed
clears the input activity marker. When an inactive processor is skipped, its
scalar ramps are synchronized to their new targets so later reactivation cannot
resume stale parameter state.

## Leveler

The input Leveler measures linked RMS energy from its internal detector signal
after Width and before the shared Input Gain stage. The detector therefore never
observes the correction that it controls. The slow path uses a continuous
1.5-second energy estimate and a six-second gain controller to establish the
long-term average shown by the Input Level control. The independent fast path
uses a 250-millisecond energy estimate and a 400-millisecond gain controller.
Its correction is added over the slow level and limited to `-6..6 dB`, so it can
follow medium-term motion without behaving like a peak compressor. As the slow
path reaches the average, the fast correction naturally returns to zero.
The slow path has a detector gate rather than an audio gate: blocks at or below
`-60 dBFS` stop changing its energy estimate and gain, and signal at or above
`-57 dBFS` opens it again. The 3 dB hysteresis prevents threshold chatter. This
gate never mutes or attenuates audio. When it closes, the fast detector also
stops learning and its correction returns smoothly to `0 dB` through the
400-millisecond controller. Pauses therefore retain the slow Level value
without being lifted by the fast `+6 dB` limit.
The bounded `-36..36 dB` Input Gain range is only the public control safety
limit. Gain is applied as a smooth multiplicative ramp across every audio
vector, preserving peaks and short musical dynamics. Detector energy below
`-120 dBFS` is treated as technical digital silence, so silence does not drive
the gain upward and the threshold does not act as a musical gate.

The processor allocates no memory and performs no locking, logging, callbacks,
or Max calls on the audio thread. Disabling Leveler stops detector/controller
updates while Managed's dedicated Leveler command commits this instance's own
latest slow level as Input Gain. Grouped control never copies another
instance's correction into this processor. The fast overlay is then removed
through the ordinary input gain ramp, and the resulting DSP snapshot restores
static gain behavior without an instantaneous step. Enabling Leveler starts
fresh slow and fast energy estimates from the retained static gain.

The slow telemetry value is the slow controller itself and directly drives the
Input Level knob. The fast telemetry value is the separately applied overlay,
not an absolute gain. It stays within `-6..6 dB` and returns toward zero around
the slow controller's moving operating point.

## Testing contract

The native foundation tests verify:

- preparation retains sample rate and maximum frame count;
- the complete identity graph preserves main audio;
- `InputLevel` applies the expected dB gain to stereo audio;
- input gain bypass preserves the original signal;
- detector buffers copy current main audio without becoming output;
- an oversized block is rejected without partial processing;
- initial and later snapshot publications are consumed directly by the
  audio-owned runtime without a Managed output callback;
- input gain retargeting preserves its current ramp position;
- Leveler converges from a sustained RMS level to its target, limits the fast
  correction to `-6..6 dB`, returns that correction to zero as the slow
  controller settles, gates slow learning below `-60 dBFS` with 3 dB hysteresis,
  releases fast correction when that gate closes, keeps short-excursion response
  bounded, holds its gain when disabled, and measures before its own gain
  correction;
- the common scalar ramp advances by sample count, preserves unchanged targets,
  retargets from its current value and resets to its target;
- detector routes use processor-owned buffers;
- compressor parallel processing applies output gain before adding wet to dry;
- compressor parallel summing leaves channels excluded by the plan untouched;
- neutralizing and reactivating Saturator does not restore stale output gain;
- equal Thick/Air identity branches preserve unity.

Additional audio algorithms, latency validation, and Max host integration are
outside this scope.
