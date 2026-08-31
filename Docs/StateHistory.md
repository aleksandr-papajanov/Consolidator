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
buckets and effective delta ranges before the common state-change observer
publishes the UI notification. Audibility does not depend on topology.

Bank-owned notifications are addressed only to instances currently focused on
the changed `BankAddress`. Instance-owned notifications return to the owning
instance. A write batch or recursive reset and every peer value reached by that
operation commit in one transaction; each changed value then produces its own
correctly addressed notification after the complete operation is visible.
Reset propagation is not delta propagation: every peer reached by a grouped
reset is prepared with that peer's own initial value. This allows peers that
currently contain different values to return independently to their defaults.
The reset protocol has a separate `group_instance` scope for a whole-instance
reset. It resolves the selected bank group to instance IDs and recursively
resets the complete DSP tree of every member, including all equalizer banks.
The ordinary `group` scope remains path-scoped and is used for processor,
equalizer-bank and filter resets.

Target snapshots and `state_changed` entries publish both the physical range
and the current effective absolute range. Analyzer and dial presenters clamp a
gesture to that effective range before sending it, while Managed remains the
authoritative validator. Topology and peer-value changes update the cached
intersection before the corresponding state notification is encoded. When a
topology change alters a range without changing its value, Managed publishes a
metadata-only `state_changed` entry with the current value and new range; it
does not create history, registry, analyzer, or DSP work.

For instance-owned processor controls, target snapshots calculate the
effective range in the context of the bank selected by the requesting UI. The
same focused-bank context remains active for the complete write or reset, so
validation and peer mutation use the exact group presented to the editor.
Managed resolves that context from the source instance's selection state and
maps it to peer instances internally. This keeps processor controls, resets,
and marker presentation on the same bank-group resolution.

The UI does not send row, instance, or bank addresses as part of a relative
state mutation. Managed takes the source instance from the transport and uses
its `SelectionContext` to resolve the selected bank and group. Rendered rows
are presentation data only. Explicit instance targeting is reserved for
commands whose purpose is to inspect or select another instance and for the
separate topology-membership contract, where the selected bank set is the
payload rather than an implicit editing target.

## Peers and constraints

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

Each symmetric bank peer component caches one intersection of its values'
remaining physical ranges. The cache is rebuilt when topology changes and
recalculated once after committed values change. A write validates against the
same materialized peer set before preparing the transaction. Topology refresh
publishes only effective ranges that actually changed.

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

`AudibilityObserver` stores only the observed local instance mute/solo values
and target `DspRuntimeState` references. On mute, solo or instance lifecycle
changes it recalculates global audibility: mute always disables its instance,
and any active solo disables every non-solo instance. Bank groups select the
instances mutated by an explicit control command; they are not part of the
audibility calculation. The former general
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

History navigation refreshes the complete observed target. An analyzer target
transition cancels any local preview gesture, clears its transaction state and
discards control-side preview coordinates before the authoritative snapshot is
presented. A stale equalizer preview therefore cannot mask the restored curve or
prevent the next gesture from opening a new history point.

Each analyzer gesture owns exactly one history point. Its final write and
`end_history` frame are sent in FIFO order, while the JavaScript binding releases
the gesture transaction immediately after dispatch instead of waiting for the
write response. A delayed response therefore cannot merge later equalizer
gestures into the previous history point.

A persistence restore uses the same transaction mechanism but replaces every
slot of each persisted value with one validated baseline. Slots are replaced
even when the active slot already equals the restored value; observers still
receive only effective active-value changes. This prevents later history
navigation from exposing values that predate the loaded snapshot.

Root removal disposes its values while the shared operation gate is still held.
This unregisters them from `StateHistory` and observer registries before another
control operation can begin. Managed publisher and command-gate disposal may
then finish outside the shared gate without leaving stopped values visible.
