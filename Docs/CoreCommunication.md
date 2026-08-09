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

User-facing DSP state is also part of `Source/Core/State`: one file per device
state (`GainState`, `SaturatorState`, `CompressorState`, `EqualizerState`, and
`FilterState`). DSP runtime and meter state remain private implementation
details of their respective devices under `Source/Dsp`.

## Parameter domain

The shared parameter model lives in `Source/Core/Parameters`:

- device, bank, filter, and parameter identifiers;
- `ParameterValue`;
- `StatePath` — единственный адресный тип состояния;
- `DspParameter<T>`.

These types are in Core because commands, instance state, coordination, and
DSP processing all use them. They retain the `consolidator::dsp` namespace to
express their DSP domain meaning.

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
bank segment directly in each target `StatePath` before publication.

## State protocol

Addressable state uses one bidirectional `StateCommand` protocol. A command
contains a `StateOperation` (`Read` or `Write`) and a bounded `StateMessage` of
`StateEntry` values. `StatePath` is a prefix query for reads and a complete
address for writes. State access is intentionally direct: `InstanceState` owns
topology reads/writes and `DspChain` owns DSP reads/writes. There is no generic
state-tree interface or visitor layer.

`StatePath` provides named factories for valid address domains:

```cpp
StatePath::Instance(instanceId);
StatePath::SelectedBank(instanceId);
StatePath::BankGroup(instanceId, bankId);
StatePath::DspParameter(instanceId, route);
```

Topology factories do not carry device or parameter identifiers. `WithNode()`
asserts that the fixed routing capacity is not exceeded.

DSP parameter writes are no longer a separate command type.
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

Responses use one common coordinator queue. The audio thread writes to the
instance-local SPSC response queue; the coordinator worker drains local
responses into the common queue. A bounded retry FIFO on each instance keeps a
temporarily full response queue from immediately losing a state response.

`latest value wins` for repeated DSP parameter updates is intentionally not yet
implemented. The next optimization is a per-instance parameter mailbox keyed
by parameter address, while rare event commands continue to use the SPSC
queue.

## State reads and writes

`InstanceCoordinator` is the command/response boundary; `InstanceRegistry`
remains only an index of instance references and group membership. It never
stores a copy of an instance or DSP state.

All state access uses one protocol:

```text
StateCommand { Read | Write, StateMessage }
  -> coordinator global queue
  -> instance local queue
  -> StateCommandHandler on the audio owner
  -> StateResponse { requestId, responseInstanceId, appliedInstanceId, operation,
                     responseIndex, responseCount, isFinal, truncated }
  -> coordinator response queue
```

For reads, `StatePath` is a prefix path. The handler represents a full-instance
read explicitly with `StatePath::Instance(instanceId)`; an instance, device,
bank-node, or parameter path narrows the result. `InstanceState` appends
topology entries, while `DspChain` filters by `deviceId` and delegates to the
matching `DspDevice`.

Read requests use a small bounded entry list; responses use a larger bounded
`StateResponseEntries` list. `FixedStateList::truncated` is set on overflow and
`StateResponse::truncated` exposes that condition to the caller.

Write results use one explicit status:

```cpp
enum class StateWriteStatus
{
    NotHandled,
    Applied,
    Unchanged,
    Rejected
};
```

`NotHandled` allows routing to continue; `Applied` and `Unchanged` return the
current value; `Rejected` identifies a recognized but invalid write. A
`std::monostate` state value clears an optional `GroupId`.

One linked write may produce multiple responses. `responseIndex`,
`responseCount`, and `isFinal` make completion explicit, while
`appliedInstanceId` identifies the instance whose state changed.
`StateResponseCollector` owns numbering and publication of coordinator response
parts. `StateRouter` contains cross-instance equalizer target resolution and
bank-path rewriting.

DSP values use `DspParameter<T>` and are exported by their owning device.
Topology values (`InstanceId`, banks, and groups) are exported directly by
`InstanceState`.

## State protocol tests

`Tests/Core/StateProtocolIntegrationTest.cpp` exercises the protocol across
two instances. It covers all top-level DSP devices, EQ parameters on different
banks and filters, grouped-bank fan-out, batch writes, unlinking with
`std::monostate`, applied-value reads, multipart responses, and empty responses
for unhandled writes. It is registered as `StateProtocolIntegrationTest` in
CTest.

## Deployment

The external build copies `ConsolidatorCore.dll` beside the generated Max
module. Deploying multiple externals therefore requires deploying that single
shared Core DLL with them; statically embedding Core into each external would
create separate singleton coordinators and is unsupported.
