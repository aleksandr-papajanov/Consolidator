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
assigns edit scope and notification ownership and appends three common observers:

- `StatePeerObserver` materializes peers, intercepts writes and maintains the
  effective delta range;
- `StateChangeObserver<TValue>` publishes effective changes through
  `IStateChangeSink` for protocol/UI delivery.
- `DspStateObserver<TValue>` records the owning instance for deferred DSP
  snapshot publication. Analyzer curves are rebuilt by the JavaScript presenter
  from the state notification received by the focused UI.

Concrete state models add their specific observers explicitly:

- `StateProjectionObserver<TValue>` projects one value into `DspRuntimeState`;
- `AudibilityObserver` observes instance mute and solo values;
- `StateTopologyObserver` observes each bank group value.

Every instance starts with user-facing bank 7 in group 0, the first editable
group. All other banks start ungrouped. This is an authoritative initial state
value, so topology indexing, registry snapshots and UI presentation observe the
membership through the ordinary state path.

Observers receive the initial value through `Attach`. `ValueChanged` is called
only after an effective direct write, committed transaction or history jump.
No-op values do not produce observer calls. `Detach` runs when the root is
removed and unregisters the value from history, peers and derived observers.
When a value is registered after history navigation has moved the cursor, it is
initialized at the history's current slot before it can receive writes. This
keeps late-created instance state, including labels, in the active history
timeline.

## Event order

Observers are ordered deliberately:

```text
effective StateValue change
  -> value-specific projection/topology/audibility observer
  -> StatePeerObserver constraint refresh
  -> StateChangeObserver
  -> DspStateObserver marks derived DSP dirty
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
instance. A write batch or recursive reset and every peer value reached by that
operation commit in one transaction; each changed value then produces its own
correctly addressed notification after the complete operation is visible.

Target snapshots and `state_changed` entries publish both the physical range
and the current effective absolute range. Analyzer and dial presenters clamp a
gesture to that effective range before sending it, while Managed remains the
authoritative validator. Topology and peer-value changes update the cached
intersection before the corresponding state notification is encoded. When a
topology change alters a range without changing its value, Managed publishes a
metadata-only `state_changed` entry with the current value and new range; it
does not create history, registry, analyzer, or DSP work.

For instance-owned controls, target snapshots calculate the effective range in
the context of the bank selected by the requesting external. The same selected
bank context remains active for the complete write or reset, including a write
to another source instance, so validation and peer mutation use the group that
the editor actually sees.

## Peers and constraints

`StatePeerObserver` is the materialized registry of observed values. Local
values keep themselves as their only peer. Connected bank values are bucketed
by value type, bank-relative `StatePath` and connected `BankAddress` values, so
different bank indexes in one topology component still address corresponding
state. Connected non-bank values use their exact path and resolve the peer
instances of the editing UI's selected bank on demand. Their state remains
instance-owned, but editing a
grouped focused bank applies the same operation to the corresponding values of
that bank's group. A focused ungrouped bank keeps the value local. Switching
focus only changes the context used by the next edit or snapshot; it does not
rebuild peers or copy or replace stored instance values.

Both bank-relative and instance-relative values are indexed by their complete
peer address. Registration, removal and topology refreshes
therefore resolve only the members of each peer set; they never rescan every
state value for every control. A separate per-instance value index limits a
topology refresh to affected state trees, so loading another instance does not
walk unrelated values owned by instances that are already running.
Bank-owned peer sets are materialized only when instances or bank-group
membership change. Instance-owned presentation peer sets are materialized for
every bank context of affected instances and reused by both snapshots and
writes. Focus changes rebuild only the affected instance's contexts. Value
changes use a reverse index to recalculate every containing context once after
commit.
State-change delivery likewise groups recipients by their selected-bank context
and encodes the corresponding effective range for each group; there is no single
focus-dependent range cache shared by different UIs.

`CopyValue` writes the requested value to every peer. `ApplyDelta` writes the
requested source value and applies the same delta to every peer. Delta mode is
valid only for `float` values with a physical range.

That physical range is the single numeric-bound policy for a delta value.
Values are never normalized or clamped: a write outside the effective range is
rejected before the transaction commits.

Each symmetric bank peer component caches one intersection of its values'
remaining physical ranges. A directional instance-owned peer set caches the
intersection for its owning editor instead, because another instance may have
a different bank selected. The caches are rebuilt when topology or focus
changes and recalculated once after committed values change. A target snapshot
also resolves the selected-bank intersection directly, and a write validates
against that same contextual peer set before preparing the transaction.
Topology refresh publishes only effective ranges that actually changed.

The Max dial binding sends a full presentation when structure, range, active
state, or visualization metadata changes. Ordinary value changes use a single
`set` delta. Dial controls apply full presentation batches with redraw
suppressed until the final batch message, so one presentation update produces
one `mgraphics.redraw()` call.

## Topology and audibility

`TopologyIndex` lives in the shared `Core/Topology` scope. It is a derived view
of bank-group and focused-bank state used by both state observers and Routing;
neither consumer owns the implementation.

`StateTopologyObserver` owns topology lifecycle:

- adding an instance indexes its current bank groups and materializes all peer
  components once after the complete state tree is attached;
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
tree. The ordered collection keeps a direct node index, making registration
and removal O(1) without scanning values owned by previously loaded instances.

`AdvanceHistoryPoint` copies the current slot into the next slot before a
logical edit. `JumpToHistory` moves the shared logical cursor in one operation,
then notifies observers for values whose effective value changed. Each successful
advance or jump increments the history revision and emits a snapshot containing
the revision, cursor, entry count and navigation availability. The protocol
publisher sends that snapshot to every currently registered instance.

`StateHistoryTransaction` prepares every write batch or recursive reset,
including every affected peer, before committing storage. Value observers run
only after all entries commit and pending state is cleared. If
preparation or storage commit fails, entries roll back in reverse order and no
observer event is emitted. Protocol delivery failures are caught by
`StateChangePublisher` and cannot turn a committed state operation into a
failed mutation.

Root removal disposes its values while the shared operation gate is still held.
This unregisters them from `StateHistory` and observer registries before another
control operation can begin. Managed publisher and command-gate disposal may
then finish outside the shared gate without leaving stopped values visible.
