# Core communication architecture

## Purpose

`ConsolidatorCore` is built as a shared DLL. Every loaded Max external links to
that DLL, so all `ConsolidatorInstance` objects use one
`InstanceCoordinator` singleton, one registry, and one coordinator worker.

## Instance lifecycle

`ConsolidatorInstance` creates its standard DSP chain and registers itself with
the coordinator in its constructor. The coordinator assigns `InstanceId` and
adds the instance to `InstanceRegistry`.

During destruction, an instance unregisters itself before its DSP chain and
queues are destroyed. Registry access is serialized with the coordinator's
registry mutex, so routing cannot retain a pointer while unregistration is in
progress.

## State and bank topology

`InstanceState` contains:

- the instance identifier;
- seven `BankState` entries (`Bank0` through `Bank6`);
- the currently selected bank.

Each `BankState` contains its `BankId` and an optional `GroupId`. A group is a
relationship between arbitrary banks, rather than between whole instances. For
example, one group may contain Bank2 of instance A, Bank5 of instance B, and
Bank4 of instance C.

`InstanceRegistry` is an index for fast lookup. Besides `InstanceId → instance`
it caches `GroupId → BankAddress[]`, where `BankAddress` is an `(InstanceId,
BankId)` pair. Group-management commands will update both the bank state and
this index.

## Command flow

The control and audio responsibilities are separated:

```text
Max / instance control code
  -> ConsolidatorInstance::EnqueueCommand()
  -> InstanceCoordinator global queue
  -> coordinator worker routes command by bank topology
  -> target instance local SPSC queue
  -> audio thread: ConsolidatorInstance::Process()
  -> local handler
  -> DSP chain
```

An instance's public `EnqueueCommand()` only forwards a command to the global
coordinator queue. It does not mutate DSP state.

The coordinator owns the only consumer worker for the global queue. It decides
whether a DSP command applies to the source instance only or to every bank in
the selected bank's group. For linked equalizer banks, it rewrites the bank
segment of `ParameterRoute` for each target bank before publication.

## DSP commands

`DspParameterChangeCommand` is the first supported command. Its execution is
implemented by `Instance/Handlers/DspParameterChangeCommandHandler.cpp`.
`ConsolidatorInstance` does not expose DSP parameter application as public API.

At the beginning of every audio callback, `ConsolidatorInstance::Process()`
drains the local queue and invokes handlers. DSP state is therefore changed on
the same audio thread that subsequently calls `DspChain::Process()`.

## Threading and queue policy

The global coordinator queue is mutex-based because it is not consumed by an
audio thread. The local queue is a fixed-capacity single-producer/
single-consumer lock-free queue:

- producer: the coordinator worker;
- consumer: the target instance's audio callback.

The audio thread takes no mutex while draining its local queue. If that queue
is full, the command is retained by the coordinator as a pending delivery and
is retried by the worker; it is not silently discarded.

`latest value wins` for repeated DSP parameter updates is intentionally not yet
implemented. The next optimization is a per-instance parameter mailbox keyed
by parameter address, while rare event commands continue to use the SPSC
queue.

## Deployment

The external build copies `ConsolidatorCore.dll` beside the generated Max
module. Deploying multiple externals therefore requires deploying that single
shared Core DLL with them; statically embedding Core into each external would
create separate singleton coordinators and is unsupported.
