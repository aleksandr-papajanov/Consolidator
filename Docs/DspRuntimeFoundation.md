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
  [`DspImplementation.md`](DspImplementation.md).
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
       -> compression
       -> fresh detector EQ -> transient shaper (attack, sustain, autogain)
       -> compressor output gain
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
[`AdaptiveCompression.md`](AdaptiveCompression.md).

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
Compressor captures the wet-branch input for compression and separately
captures after compression/makeup for transient shaping. Each detector
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
once per block. Saturator output, Compressor output and final Output level use
`DspGainRamp`, which delegates sample progression to `DspParameterRamp`. A
transition back to neutral continues until unity is reached even after Managed
clears the input activity marker. Saturator, Compressor and Polish continue
processing through their wet-to-dry transition before resetting runtime history.

## Transient shaper

The transient shaper uses the compressor's private detector signal as a linked
control envelope and never analyzes the already modified main output. A common
5 ms RMS envelope feeds symmetric 2 ms and 25 ms followers. Both followers have
the same DC gain, avoiding the unequal peak bias of asymmetric detectors.

Attack uses 1.25 times the fast/slow envelope difference in dB, up to 7.5 dB.
Sustain has its own 120 ms reference and uses 1.5 times its positive difference
above the fast envelope, up to 9 dB. Both knobs scale their contribution linearly,
making intermediate positions more pronounced. The longer sustain reference
extends enhancement into the tail instead of sharing the 25 ms attack reference.
A gate on the smoothed RMS envelope
closes at -90 dBFS and opens 6 dB higher; individual zero crossings do not switch
the controller. Both
controls use the common parameter ramps, while the derived dB gain uses
sample-rate-independent `1 ms` attack and `50 ms` release smoothing before one
linked gain is applied to the selected channels. Neutral Attack and Sustain
therefore preserve unity, and channels excluded by the processing plan neither
affect nor receive the linked gain.

Transient shaping runs after compression and its makeup, so its enhanced
envelope is not immediately compressed again. Its own detector measures that
post-compression signal. Transient shaping and Saturator each own a linked
partial RMS autogain: 1 second pre/post energy estimates, 2 second correction
smoothing, 50% compensation in dB, limited to +/-3 dB. Learning freezes below
-100 dBFS input energy. Measurements exclude autogain and manual Output gain.
Saturator scales compensation by Drive; transient shaping uses its activity ramp.
Zero amount restores exact unity compensation. Slow partial compensation
reduces loudness bias while retaining short-term envelope changes.

## Leveler

The input Leveler measures linked RMS energy from its internal detector signal
after Width and before the shared Input Gain stage. The detector therefore never
observes the correction that it controls. The slow path uses a continuous
1.5-second energy estimate and a six-second gain controller to establish the
long-term average shown by the Input Level control. The independent fast path
uses a 250-millisecond energy estimate and a gain controller with 400 ms
downward and 800 ms upward response.
Its correction is added over the slow level and limited to `-6..6 dB`, so it can
follow medium-term motion without behaving like a peak compressor. Fast correction
is the difference between slow and fast measured levels, not the unfulfilled
correction of the slow gain controller. It returns to zero on a steady source.
The slow path has a detector gate rather than an audio gate: a continuous 50 ms
RMS estimate closes below -60 dBFS after a 200 ms hold and opens at -57 dBFS.
The gate and energy estimates advance per sample independently of host block
partitioning; gain targets still update once per block. This
gate never mutes or attenuates audio. When it closes, the fast detector also
stops learning and its correction returns smoothly to `0 dB` through the
400-millisecond controller. When it opens, both detector estimates restart from
the first valid sample energy so a pause cannot create a false fast correction.
Pauses therefore retain the slow Level value
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

## Saturator

The Saturator derives a linked 500 ms energy estimate from the maximum absolute
selected detector channel. The reference amplitude is sqrt(2 * energy), with
learning held on samples below -120 dBFS. Main audio is divided by that
reference before shaping and multiplied by it afterward. The startup average
is divided by its accumulated exponential weight, avoiding an underestimated
reference inherited from the first sample. Scaling
the same waveform up or down therefore scales the output without materially
changing its nonlinear proportion. Detector EQ can deliberately bias which
content establishes the operating level, but detector audio is never mixed into
the main path.

The smooth odd transfer family is:

```text
k = 0.75 + 1.25 * Curve
Shape(x) = sqrt(1 + k*k) * x / sqrt(1 + k*k*x*x)
```

Curve changes the knee and harmonic proportions, including at zero Curve.
The static curve preserves +/-1 and has a soft asymptotic continuation outside
that range. Drive blends the nonlinear contribution; zero Drive is identity.
First-order antiderivative antialiasing (ADAA) integrates the nonlinear residual
Shape(x)-x between adjacent samples, retaining the current dry sample. Its
antiderivative difference is rationalized to avoid small-difference cancellation.
The current reference and Curve evaluate both endpoints, so parameter changes
are not mistaken for sample differences. This reduces nonlinear aliasing without
delaying the dry path, but is not alias-free, a brick-wall limiter, or an exact
preserver of transient peaks. Saturator Output remains a separate post-shaper gain.

When `saturatorSplit` is enabled, the signal is decomposed into three
complementary bands using fixed low/mid crossover points at `250 Hz` and
`2500 Hz`. Low, mid and high are processed independently by the same transfer
function and then recombined. A ten-millisecond interpolation between full-band
and split processing prevents a mode switch from exposing reset filter history
as a click. The split path owns only scalar filter history, so it does not
allocate on the audio thread. With neutral Drive, both paths reproduce
the original signal apart from normal floating point round-off.

## Output limiter

The limiter is the final audio operation after Output gain. It measures the
largest selected-channel sample, applies instantaneous linked reduction, then
clamps each selected output sample to +/-10^(ceilingDb/20). Lowering Target
applies the new ceiling immediately when its snapshot is consumed; raising it
uses the 10 ms ramp. While enabled, finite output samples cannot exceed the
requested Target, including during its automation. Reduction releases over
80 ms. It allocates no buffers and adds no lookahead latency.
This is a sample-peak ceiling, not a true-peak limiter; intersample peaks and
distortion from large instantaneous corrections remain possible. Turning the
limiter off releases its current reduction before the graph stops processing it;
the final clamp is disabled. Output bypass and instance bypass preserve their
existing bypass semantics and do not apply the ceiling.

## Air shimmer

Thick generates harmonic content from a 20–50 Hz filtered source, uses a bounded
shaper with RMS/fast-peak normalization, calibrates the post-filter harmonic level
and applies bounded upward compression with burst reduction to that branch.
Air combines broad/top shelves with upper harmonics from a 3–6.5 kHz source;
only those harmonics receive dynamic attenuation and subtle stereo-side enhancement.
Their signal paths, normalization, ADAA and
limitations are specified in [`PolishHarmonics.md`](PolishHarmonics.md).

## Testing contract

The existing native tests predate the latest adaptive DSP changes. They have
not been updated or executed for this revision at the user's request. The
following list describes prior coverage, not verification of the new algorithms:

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
- compressor parallel processing applies output gain before normalized dry/wet interpolation;
- compressor parallel summing leaves channels excluded by the plan untouched;
- neutralizing and reactivating Saturator does not restore stale output gain;
- neutral drive and curve preserve audio while positive drive applies the curve
  to both signal polarities;
- split low/mid/high bands recombine to unity with a neutral curve;
- neutral transient shaping preserves unity and positive Attack enhances a
  linked stereo transient;
- positive Air produces a finite non-identity shimmer response;
- equal Thick/Air identity branches preserve unity.

Additional audio algorithms, latency validation, and Max host integration are
outside this scope.
