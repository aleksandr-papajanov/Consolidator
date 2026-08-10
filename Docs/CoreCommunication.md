# Core communication architecture

`ConsolidatorCore` owns one process-wide `InstanceCoordinator`, registry and
coordinator worker. Instances register their topology and coordinator-owned
`StateStore` during construction.

## Command flow

```text
Max / control code
  -> ConsolidatorInstance::EnqueueCommand()
  -> InstanceCoordinator global queue
  -> CommandRouter resolves topology and group targets
  -> StateStore::WriteState() on coordinator thread
  -> immediate StateResponse
  -> per-instance DspUpdateMailbox (latest-value slots)
  -> audio thread: ConsumeLatest()
  -> DspChain::ApplyRuntimeUpdates()
  -> DspChain::Process()
```

`StateStore` is the only authoritative owner of user-facing parameters,
physical ranges, banks, groups and selected bank. Its topology component is
`InstanceState`; DSP parameter state is stored in `ChainState`. Both are
accessed through `StateStore` and are written only by the coordinator worker.
Reads are also resolved from `StateStore`; the coordinator never reads a live
DSP chain.

`ConsolidatorInstance::Initialize()` registers the fully constructed instance,
then registers every DSP `StatePath` and publishes the complete initial runtime
state before returning to the external owner. `Process()` is not exposed by the
external wrapper before this initialization completes.

`DspUpdateMailbox` has one producer (the coordinator worker) and one consumer
(the instance audio callback). During instance initialization it registers a
fixed slot for every DSP `StatePath`. Publishing replaces the value in that
path's slot and never requires a retry queue; an unregistered path is a fatal
invariant violation. Each update carries a monotonic revision for diagnostics
and ordering within a batch.

`RegisterPath()` completes before the first `Process()` call. The mailbox is
intentionally single-producer: concurrent publishers are not supported by the
seqlock protocol.

Non-coalescable actions such as reset must use a separate event queue.

## State protocol

Addressable state uses two self-contained command types: `ReadStateCommand`
with bounded `queries` and `WriteStateCommand` with bounded `entries`.
`Command` is their variant. The `ConsolidatorInstance` fills `instanceId`
before enqueueing. `StatePath` is a prefix query for reads and a complete address
for writes. Each request produces one consolidated `StateResponse` addressed by
the same `instanceId`; it contains `requestId`, `entries`, and `truncated`.
`StateStore` owns both topology
entries and all DSP parameter entries.

Valid path factories include:

```cpp
StatePath::Instance(instanceId);
StatePath::SelectedBank(instanceId);
StatePath::BankGroup(instanceId, bankId);
StatePath::DspParameter(instanceId, route);
```

`ParameterConstraintResolver` reads only `StateStore`. Write routing and
constraint dependencies are intentionally separate:

- `GroupGraph` owns topology traversal through:
  `GetGroupMembers()`, `GetGroupedBanks()`, and `GetConnectedGroupBanks()`.
- `StateRouter::ResolveSourceBank()` selects the path's bank or the source
  instance's selected bank.
- `StateRouter::ResolveWriteTargets()` returns one direct group, falling back to
  the source bank when it is ungrouped, and collapses instance-owned targets.
- `StateRouter::ResolveConstraintTargets()` returns the direct group for
  bank-scoped paths and the connected topology component for instance-owned
  parameters.
- `StateRouter::Retarget()` adapts a path to a target instance and bank;
  `IsBankScoped()` identifies the current bank-owned EQ paths.

Constraint traversal walks connected group components transitively, so a chain
such as A → B → C is handled correctly. Group-linked writes use only the direct
group and are validated for every write target before any target is mutated,
then committed to each target store and returned in one coordinator response.
Constraint enrichment runs after the complete plan is committed.

After a parameter commit, the response also includes authoritative values for
every target returned by `ResolveConstraintTargets()`. This keeps
effective limits synchronized for other grouped instances, including writes
that remain local because the selected bank is not grouped.

One coordinator write plan publishes all runtime updates for an instance before
the response is emitted. The audio thread scans the fixed slots once at the
start of its block and stages the resulting local batch; values superseded
before that scan are collapsed by path.

`StateResponsePublisher` publishes coordinator responses. `Applied` means that
the authoritative `StateStore` changed and the instance mailbox accepted the
runtime update; it does not mean that the audio thread has already applied it.
There is no local
audio command/response queue for state access and no `ParameterStateView`.

## Core component boundaries

`Core/Domain/State` contains the authoritative state model and `StateStore`.
`Core/Routing` contains command routing, group traversal, constraint resolution
and response delivery. `InstanceCoordinator` remains the composition root and
owns the coordinator worker, command queue and routing components, but routing
does not belong to the coordinator lifecycle itself.

The state implementation is split by operation without introducing additional
state-owner objects:

```text
Core/Domain/State/
├─ StateStore.h
├─ StateStore.cpp
├─ StateStore.Factory.cpp
├─ StateStore.Read.cpp
├─ StateStore.Write.cpp
├─ StatePath.h
├─ StateEntry.h
├─ ParameterState.h
├─ InstanceState.h
├─ ChainState.h
└─ DspStates.h
```

`StateStore.cpp` owns construction, `StateStore.Factory.cpp` builds a complete
`ChainState` from `DspSettings`, and the read/write translation remains in the
two operation-specific implementation files. Command transport types live in
`Core/Domain/Commands/StateProtocolCommands.h`; they are separate from the
state address and entry representation.

## DSP boundary

DSP devices do not own authoritative user state. They receive runtime updates
from `DspUpdateMailbox`, update local `RuntimeState`, and process audio. A
runtime state may contain a local copy of target values, coefficients,
smoothing values and audio memory, but it is never read by the coordinator as
the source of truth.

Topology changes include affected-bank parameter entries in the response so
the UI receives refreshed effective ranges after group changes.

## Lifecycle

Instance unregistering is serialized by the coordinator registry mutex. Pending
runtime updates are owned by the instance mailbox and cannot be published after
the instance is removed from the registry. The DSP chain is destroyed only
after unregistering the instance.
