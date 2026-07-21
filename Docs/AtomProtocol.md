# Consolidator Atom Protocol

Runtime communication uses Max atom lists. Dictionaries are reserved for
persistence and are never used for component commands or snapshots.

## Commands

```text
command <version> <source> <requestId> <name> <fields...>
```

The current commands are `eq.set_parameter`, `eq.set_bypass`, `eq.reset_filter`,
`eq.set_section_bypass`, `eq.reset_section`, `eq.add_bank`,
`eq.remove_bank`, `eq.rename_bank`, `eq.select_bank`, `analyzer.listen`,
`gain.set_parameter`,
`compressor.set_parameter`, `compressor.set_bypass`, `compressor.reset`,
`saturator.set_parameter`, `saturator.set_bypass`, `saturator.reset`, `fit.start`, `fit.cancel`,
`fit.clear`, `fit.complete`, and `fit.fail`. All processor and EQ
values are absolute. `fit.complete` frames its filters and values with explicit
counts and is committed atomically by Host. `eq.add_bank` accepts an optional
single name atom; when omitted, Host generates the deterministic bank name.

## Events

```text
event <version> host <eventId> <name> <fields...>
```

Events include `host.initialized`, `store.updated`,
`command.rejected`, and `operation.changed`.

## EQ Snapshot

```text
snapshot <version> host eq <revision> <selectedBank> <bankCount>
    <bankId> <bankName> <preBypass> <postBypass> <filterCount>
        <filterId> <bypass> <valueCount> <absoluteValue...>
```

Bank and filter IDs are one-based. Counts frame every variable-length section.
The same atom sequence may arrive through Max as a `snapshot` selector or as a
`list` whose first atom is `snapshot`; these are two Max deliveries of one
protocol message, not two protocol formats.

Filter definitions use a second snapshot family:

```text
snapshot <version> host definitions <revision> <filterCount>
    <filterId> <pre|post> <type> <defaultBypass> <parameterCount>
        <name> <minimum> <maximum> <scale> <defaultValue>
```

Processor definitions use `snapshot 1 host processor_definitions ...` and
contain the absolute range, scale, and default for input gain, compressor
attack/release/threshold, saturator saturation, and output gain.

The audio endpoint receives the complete processing state:

```text
snapshot 1 host dsp <revision> <EQ snapshot body...>
    <inputGainDb>
    <compressorBypass> <attackMs> <releaseMs> <thresholdDb>
    <saturatorBypass> <saturation>
    <outputGainDb>
```

Analyzer, SpectrumView, Filter UI, and Approximator continue to consume the
EQ-only snapshot. Only `consolidator.dspprocessor` consumes `dsp`.

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
callback forwards only the latest complete frame to SpectrumView and the
Approximator difference input.
