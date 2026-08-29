# Bank Index Contract

All bank indexes in Consolidator are zero-based.

The only valid bank indexes are `0..6`. The same value is used without
conversion in Max, JavaScript, Native, Managed, registry messages, state
paths, target snapshots, topology, and group targets.

No layer may add or subtract one when forwarding a bank index. A bank index is
not a display number; labels may use any presentation text independently.

This applies to registry snapshots and deltas, state read/write paths,
`observe_target`, target snapshots, equalizer bank paths, and instance group
targets for Solo/Mute.

Filter identifiers are a separate wire contract. Equalizer filters remain
one-based `1..7`, and detector filters remain one-based `1..2`; Managed converts
those identifiers to zero-based array indexes only while decoding a state path.
