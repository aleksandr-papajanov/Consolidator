# Managed State and Observers

The state tree is the authoritative Managed store. The top-level `State`
scope contains the reusable mechanism; `Core/State` defines this application's
concrete values and reactions. `ManagedInstance` owns the corresponding
`ManagedState`, DSP runtime and native publisher.

The infrastructure boundary is documented separately in
[StateInfrastructure.md](StateInfrastructure.md).

Application state code is organized by responsibility:

```text
Core/State/
├─ Models/      concrete instance and DSP state composition
├─ Observers/   topology, peer, audibility, projection and UI reactions
├─ StateValueFactory.cs
├─ StateNodeIds.cs
└─ domain value types and edit policies

Core/Topology/
└─ TopologyIndex.cs  shared derived bank/group indexes and queries
```

## State values

Each history-backed `StateValue<TValue>` owns `StateHistory.Capacity` slots and
an ordered observer list. Generic
`StateRegistry<TRootId>` owns roots and registers values with observers supplied
by its caller. It has no dependency on Core, instances, topology, UI ownership
or peer policy.

`Core.State.StateValueFactory` creates the concrete application values. It
assigns edit scope and notification ownership and appends two common observers:

- `StatePeerObserver` materializes peers, intercepts writes and maintains the
  effective delta range;
- `StateChangeObserver<TValue>` publishes effective changes through
  `IStateChangeSink` for protocol/UI delivery.

Concrete state models add their specific observers explicitly:

- `StateProjectionObserver<TValue>` projects one value into `DspRuntimeState`;
- `AudibilityObserver` observes instance mute and solo values;
- `StateTopologyObserver` observes each bank group value.

Observers receive the initial value through `Attach`. `ValueChanged` is called
only after an effective direct write, committed transaction or history jump.
No-op values do not produce observer calls. `Detach` runs when the root is
removed and unregisters the value from history, peers and derived observers.

## Event order

Observers are ordered deliberately:

```text
effective StateValue change
  -> value-specific projection/topology/audibility observer
  -> StatePeerObserver constraint refresh
  -> StateChangeObserver
  -> IStateChangeSink
  -> StateChangeRouter
  -> focused UI instances
```

For a bank group change, `StateTopologyObserver` first updates
`TopologyIndex`. It then asks `StatePeerObserver` to rebuild affected peer
buckets and effective delta ranges, refreshes audibility, and only afterwards
does the common state-change observer publish the UI notification.

Bank-owned notifications are addressed only to instances currently focused on
the changed `BankAddress`. Instance-owned notifications return to the owning
instance. A grouped write commits every peer value in one transaction; each
changed bank value then produces its own correctly addressed notification.

The current wire protocol publishes values, not physical/effective ranges.
Topology changes update cached limits inside Managed immediately, while UI
receives the bank-group value change. A future UI range display requires an
explicit range notification contract rather than unchanged value events.

## Peers and constraints

`StatePeerObserver` is the materialized registry of observed values. Local
values keep themselves as their only peer. Connected bank values are bucketed
by value type, bank-relative `StatePath` and connected `BankAddress` values, so
different bank indexes in one topology component still address corresponding
state. Connected non-bank values use their exact path and the connected
instance component derived from all grouped banks of the owning instance.
They do not depend on that instance's current UI focus, and an ungrouped value
always keeps itself as a writable peer.

`CopyValue` writes the requested value to every peer. `ApplyDelta` writes the
requested source value and applies the same delta to every peer. Delta mode is
valid only for `float` values with a physical range.

That physical range is the single numeric-bound policy for a delta value.
Values are never normalized or clamped: a write outside the effective range is
rejected before the transaction commits.

Each observed delta value caches the intersection of its peers' remaining
physical ranges. The cache is rebuilt when topology changes and recalculated
after a peer value changes. A write validates the delta against that cache
before preparing the transaction.

## Topology and audibility

`TopologyIndex` lives in the shared `Core/Topology` scope. It is a derived view
of bank-group and focused-bank state used by both state observers and Routing;
neither consumer owns the implementation.

`StateTopologyObserver` owns topology lifecycle:

- adding an instance indexes its current bank groups;
- changing a group moves the bank between group buckets;
- removing an instance removes its buckets before surviving peers refresh.

The group value is bank-owned for notification routing but has local edit
scope. Changing membership rebuilds peer buckets; it is not itself propagated
as a grouped DSP edit.

`AudibilityObserver` stores only the observed mute/solo values and target
`DspRuntimeState` references. On mute, solo or topology changes it recalculates
audibility for the affected connectivity graph. The former general
`StateValueProjection`, `AudibilityResolver` and `ProcessingStateResolver`
layers no longer exist.

## DSP projection

DSP state constructors attach `StateProjectionObserver<TValue>` directly to
the values that affect the runtime snapshot. The observer applies the initial
value and every later effective change. Bypass observers update both the raw
bypass marker and its derived active flag; detector listen and equalizer
bank/filter activity are projected by their owning state objects.

These observers run only on the Managed control path. Native audio reads the
published fixed-layout snapshot and never accesses observers or the state tree.

## History and transactions

`StateHistory` owns one logical cursor and a deterministic registration-order
list of active `IHistoryValue` objects. The generic registry registers and
unregisters values directly, so history never discovers values by walking the
tree.

`AdvanceHistoryPoint` copies the current slot into the next slot before a
logical edit. `JumpToHistory` moves the shared logical cursor in one operation,
then notifies observers for values whose effective value changed. Each successful
advance or jump increments the history revision and emits a snapshot containing
the revision, cursor, entry count and navigation availability. The protocol
publisher sends that snapshot to every currently registered instance.

`StateHistoryTransaction` prepares every peer before committing storage. Value
observers run only after all entries commit and pending state is cleared. If
preparation or storage commit fails, entries roll back in reverse order and no
observer event is emitted. Protocol delivery failures are caught by
`StateChangePublisher` and cannot turn a committed state operation into a
failed mutation.

Root removal disposes its values while the shared operation gate is still held.
This unregisters them from `StateHistory` and observer registries before another
control operation can begin. Managed publisher and command-gate disposal may
then finish outside the shared gate without leaving stopped values visible.
