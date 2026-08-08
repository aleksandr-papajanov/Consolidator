# Consolidator Atom Protocol

Runtime communication uses Max atom lists. Dictionaries are reserved for
persistence and are never used for component commands or snapshots.

## Commands

```text
command <version> <source> <requestId> <name> <fields...>
```

The current commands are `eq.set_parameter`, `eq.set_bypass`,
`eq.set_chain_bypass`, `eq.set_chain_solo`, `eq.reset`, `eq.reset_all`, `eq.join_banks`,
`eq.commit_hidden`, `eq.commit_all`, `eq.set_link`, `eq.select_bank`,
`gain.set_parameter`, `compressor.set_parameter`, `compressor.set_bypass`,
`compressor.set_detector_parameter`, `compressor.set_detector_listen`,
`compressor.reset`, `saturator.set_parameter`, `saturator.set_bypass`,
`saturator.set_detector_parameter`, `saturator.set_detector_listen`,
`saturator.reset`, `analyzer.clear`, `analyzer.set_view`, `fit.start`,
`fit.cancel`, `fit.clear`, `fit.complete`, and `fit.fail`. All processor and EQ
values are absolute. EQ fit results carry explicit filter counts and update only
the EQ store; processor state is not part of an EQ fit transaction.

## Events

```text
event <version> host <eventId> <name> <fields...>
```

Events include `host.initialized`, `store.updated`, `parameter.updated`,
`command.rejected`, `fit.requested`, `analyzer.view_changed`, and
`operation.changed`. Fit requests are framed as:

```text
fit.requested <sessionId> <bankId> <residual|absolute> <pointCount> <curveDb...>
```

`residual` is combined with the current selected-bank response. `absolute` is
already the complete target response and is fitted directly.

## EQ Snapshot

```text
snapshot <version> host eq <revision> <selectedBank> <bankCount>
    <bankId> <bankName> <bypass> <filterCount>
        <FilterId> <bypass> <valueCount> <absoluteValue...>
```

EQ has hidden system bank `0` and fixed user banks `1..6`; counts frame every
variable-length section.
The same atom sequence may arrive through Max as a `snapshot` selector or as a
`list` whose first atom is `snapshot`; these are two Max deliveries of one
protocol message, not two protocol formats.

The audio endpoint receives the complete processing state:

```text
snapshot 1 host dsp <revision> <EQ snapshot body...>
    <inputGainDb>
    <compressorBypass> <attackMs> <releaseMs> <inputDb> <outputDb>
    <compressorMix> <compressorMode> <compressorDetectorFields...>
    <saturatorBypass> <inputDb> <outputDb> <saturatorMix> <saturatorMode>
    <saturatorDetectorFields...>
    <outputGainDb>
```

Analyzer and SpectrumView consume the EQ-only snapshot. DspProcessor consumes
the complete DSP snapshot. Approximator receives compact EQ and processor
snapshots only to construct its EQ result; it never applies processor values.

## Persistence

Persistence is the only Dictionary boundary. `MaxDictionarySerializer` converts
the persisted typed state to and from a Max Dictionary while the device owns
the dictionary lifetime. The Max transport message is `dictionary <name>` and
must reach `pattrstorage` synchronously; only state preparation may be
debounced, never the temporary dictionary name. Host store events are emitted
immediately, repeated EQ snapshots are coalesced to the latest revision on the
Max main thread, and persistence serialization runs after a restartable 100 ms
debounce.

## Streaming Boundary

FFT frames are not protocol messages and do not pass through Host. Analyzer's
audio thread publishes immutable curve frames through a preallocated
single-producer/single-consumer triple buffer. When the consumer is behind, a
new frame replaces the previous unconsumed frame. A single coalesced Max queue
callback forwards only the latest complete frame to SpectrumView. Approximator
receives one complete target curve in `fit.requested`; audio frames and fit
candidates never pass through Host.
