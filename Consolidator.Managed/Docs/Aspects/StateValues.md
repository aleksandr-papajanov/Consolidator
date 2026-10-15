# State values and observer order

The state tree is the authoritative Managed store. The top-level `State`
scope contains the reusable mechanism; `Core/State` defines this application's
concrete values and reactions. `ManagedInstance` owns the corresponding
`ManagedState`, DSP runtime and native publisher.

The infrastructure boundary is documented separately in
[StateInfrastructure.md](StateInfrastructure.md).

Application state code is organized by responsibility:

```text
Core/State/
├─ Identifiers/ domain identifiers and addresses
├─ Definitions/ value ranges, definitions and edit policies
├─ Values/      value factory, metadata and change context
├─ Composition/ model builder and construction context
├─ Models/
│  ├─ Instance/    instance state and transient selection
│  ├─ Dsp/         DSP aggregate and processor state
│  └─ Filters/     filter state records
├─ Observers/   topology, peer, audibility, projection and UI reactions
└─ StateNodeIds.cs  state tree schema

Core/Topology/
└─ TopologyIndex.cs  shared derived bank/group indexes and queries
```

## State values

Each history-backed `StateValue<TValue>` owns `StateHistory.Capacity` slots and
an ordered observer list. Generic
`StateRegistry<TRootId>` owns roots and registers values with observers supplied
by its caller. It has no dependency on Core, instances, topology, UI ownership
or peer policy.

`Core.State.StateValueFactory` creates the concrete application values. A
`StateValueDefinition<TValue>` from the typed
`StateValueDefinitions` catalog supplies the default, edit mode,
physical range and history participation; the factory assigns edit scope and
notification ownership and appends three common observers:

- `StatePeerObserver` materializes peers, intercepts writes and maintains the
  effective delta range;
- `StateChangeObserver<TValue>` publishes effective changes through
  `IStateChangeSink` for protocol/UI delivery.
- `DspStateObserver<TValue>` records the owning instance for deferred DSP
  snapshot publication. Analyzer curves are rebuilt by the frontend presenter
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
  -> value-specific projection/topology/solo observer
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

