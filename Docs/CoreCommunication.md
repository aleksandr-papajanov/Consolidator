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

Addressable state uses one bidirectional `StateCommand` protocol containing a
`StateOperation` and bounded `StateEntry` list. `StatePath` is a prefix query
for reads and a complete address for writes. `StateStore` owns both topology
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

- `ResolveWriteTargets()` routes an instance-owned parameter through the group
  of the source instance's selected bank.
- `ResolveConstraintDependencies()` inspects all grouped banks of the source
  instance when calculating effective limits.

Both traversals walk connected group components transitively, so a chain such
as A → B → C is handled correctly for its respective purpose. Group-linked
writes are validated for every write target before any target is mutated, then
committed to each target store and returned in one coordinator response.
Constraint enrichment runs after the complete plan is committed.

After a parameter commit, the response also includes authoritative values for
every dependency returned by `ResolveConstraintDependencies()`. This keeps
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
