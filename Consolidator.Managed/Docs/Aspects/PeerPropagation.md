# Peer propagation and constraints

`StatePeerObserver` is the materialized registry of observed values. Local
values keep themselves as their only peer. Connected bank values are bucketed
by value type, bank-relative `StatePath` and connected `BankAddress` values, so
different bank indexes in one exact bank group still address corresponding
state. Connected non-bank processor values remain instance-owned and use their
exact path plus the current operation's focused bank to resolve the matching
instances. An ungrouped or absent focused bank keeps the mutation local.

Bank-relative and instance-relative values are indexed by their complete peer
address. Bank-owned peer sets are materialized when topology changes.
Instance-owned contextual peer sets are cached only for requested focused-bank
contexts and reused by snapshots, notifications and writes. Topology refreshes
rebuild only cached sets whose membership can have changed.

`CopyValue` writes the requested value to every peer. `ApplyDelta` writes the
requested source value and applies the same delta to every peer. Delta mode is
valid only for `float` values with a physical range.

That physical range is the single numeric-bound policy for a delta value.
Values are never normalized or clamped: a write outside the effective range is
rejected before the transaction commits.

The target-state client selects the wire edit mode from the current scope and
snapshot metadata. A ranged value edited in group scope is sent as `delta`, so
each peer retains its offset and the effective absolute range remains stable
throughout the gesture. Local edits and non-ranged group values are sent as
`copy`.

Each symmetric bank peer component caches one intersection of its values'
remaining physical ranges. The cache is rebuilt when topology changes and
recalculated once after committed values change. A write validates against the
same materialized peer set before preparing the transaction. Topology refresh
publishes only effective ranges that actually changed.

Bindings apply snapshot metadata as one presentation batch. Ordinary value changes update existing models; readiness changes are not repeated per value.
