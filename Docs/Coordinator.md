# Managed Coordination

## Shared services

Managed coordination services are DI singletons for the lifetime of the loaded
Managed DLL:

```text
ManagedServices
  -> InstanceRegistry
  -> StateRegistry<InstanceId>
  -> StateValueFactory
  -> StateHistory
  -> TopologyIndex
  -> StatePeerObserver
  -> StateTopologyObserver
  -> AudibilityObserver
  -> InstanceControlTargetResolver
  -> FftAnalyzer
  -> InstanceCommandRouter
  -> CommandExecutor
```

Responsibilities are intentionally narrow:

- `InstanceRegistry` owns instance IDs, registration, unregistration and
  `ManagedInstance` lifetime;
- generic `StateRegistry<InstanceId>` owns only root-node creation, path
  registration and root removal;
- `StateValueFactory` owns application edit policy and composes concrete value
  observers;
- `StateHistory` owns the shared history cursor and active history-value list;
- `InstanceControlTargetResolver` resolves explicit instance or exact bank-group
  targets for mute and solo commands;
- `StatePeerObserver` owns peer buckets, grouped mutations and effective delta
  ranges;
- `StateTopologyObserver` reacts to bank-group and instance lifecycle events;
- `TopologyIndex` stores derived group/focus indexes and serves queries;
- `AudibilityObserver` observes mute/solo values and projects audibility;
- `FftAnalyzer` owns the demanded source's bounded audio capture, prepared
  sample rate, worker-side FFT, focused-viewer configuration and spectrum
  publication;
- `InstanceCommandRouter` validates sources and selects targets;
- `CommandExecutor` executes commands on already selected instances.

UI-only biquad response calculation lives in the JavaScript
`AnalyzerPresenter`; Managed does not retain analyzer coefficient or curve
caches.

There is no global coordinator facade, general projection service or state
resolver layer.

## Instance lifetime

Registration creates a root tree, runtime snapshot, typed state models and
their observers. `StateTopologyObserver.AddState` indexes the completed
instance and refreshes peer buckets before the first DSP snapshot is published.

`ManagedInstance` owns its `ManagedState`, DSP publisher and per-instance
command gate. The generic registry owns the root tree. The root owns its
history-backed values.

Unregistration runs in this order under the shared operation gate:

```text
remove ManagedInstance from InstanceRegistry
  -> remove and dispose generic registry root values
  -> unregister history and value observers
  -> remove topology and refresh surviving peers
  -> leave shared operation gate
  -> wait for the instance command gate
  -> stop DSP publisher
```

After `UnregisterInstance` returns, the instance ID is not reused and its native
callback cannot be invoked. Managed shutdown applies the same removal sequence
to every remaining instance before releasing audio handles. The NativeAOT
module remains loaded until process termination because dynamic unloading is
not supported.

## State changes

The state tree is authoritative. Effective changes are propagated by the
observer chain documented in [StateHistory.md](StateHistory.md). DSP runtime,
topology, peer constraints, audibility and protocol notifications are derived
reactions; none is a second source of truth.

The currently observed bank is transient routing metadata on each external's
instance model; it is not a tree value and is never persisted or exposed as a
`selected_bank` parameter. `observe_target` copies the view into the derived
`TopologyIndex` used for routing notifications. Bank group membership is
history-backed and observed by the same boundary.

UI `WriteStateCommand` carries an explicit instance target. Other relative
commands may route to the currently observed instance. The target `StateValue`
delegates its mutation to `StatePeerObserver`, which prepares all materialized
peers and commits them in one `StateHistoryTransaction`. The command executor
does not contain a second broadcast-write implementation. After a successful
write or reset, the executor publishes DSP snapshots for all instances because
peer and audibility observers may have changed runtime projections outside the
explicit command target.

Instance mute and solo are local instance-owned values. Dedicated control
commands carry an explicit instance or bank-group target and use
`InstanceControlTargetResolver` to select a snapshot of exact group members.
The mute handler changes only that resolved set. Exclusive solo additionally
clears solo outside the set; additive solo leaves it unchanged. These handlers
prepare all affected values in one transaction and bypass focused-bank peer
resolution. Direct protocol writes to instance mute or solo are rejected so
there is no second mutation path.

`InstanceCommandRouter` has no edit context and does not derive group targets
from focus. Focus is used only by relative bank routing and notification
addressing; grouped state propagation is limited to bank-owned peer values.

## Threading

The shared `IOperationGate` serializes complete control operations:

- register and unregister;
- protocol message execution;
- prepare operations;
- topology mutations;
- history advance and cursor jumps.

`StateHistory` also protects its cursor and active-value list with a private
lock. Observer calls occur after that lock is released. The operation gate keeps
storage, observer projection, topology, constraints and DSP publication in one
logical order.

Each `ManagedInstance` has its own command gate. The shared operation gate
prevents another control operation from interleaving while an async routed
command is suspended.

The audio callback does not enter coordination services or either control-path
gate. It uses its per-instance realtime-safe handle and the published native DSP
snapshot only.

`FftAnalyzer` is the managed audio-input singleton. One external is the active
viewer at a time. Its audio callback only writes the selected source instance
into a preallocated bounded capture block; a worker performs the FFT and sends
the source spectrum back to that viewer. The viewer and source may be different
instances. Full capture blocks are dropped when the bounded queue is full.
Spectrum output uses the `fft` protocol selector and is consumed by the existing
analyzer controls.

## Process boundary

Singleton state is shared only inside one process and one loaded Managed DLL.
Sharing state between processes would require an explicit IPC or persistence
boundary.
