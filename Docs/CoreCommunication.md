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

The shared parameter model is split between `Source/Core/Ids` and
`Source/Core/Parameters`:

- device, bank, filter, and parameter identifiers in `Source/Core/Ids`;
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
  -> InstanceCoordinator global ConcurrentQueue
  -> CommandRouter routes command by topology
  -> CommandDeliveryQueue preserves per-instance order and retries delivery
  -> target InstanceCommandQueue
  -> audio thread: ConsolidatorInstance::Process()
  -> StateCommandHandler returns StateResponse
  -> InstanceResponseQueue
  -> DSP chain
```

An instance's public `EnqueueCommand()` only forwards a command to the global
coordinator queue. It does not mutate DSP state.

The coordinator owns the only consumer worker for the global queue. Its
`CommandRouter` decides whether a DSP command applies to the source instance
only or to every bank in the selected bank's group. For linked equalizer banks,
it rewrites the bank segment directly in each target `StatePath` before
publication. `StateRouter` resolves related targets, while
`StateResponsePublisher` adds multipart response metadata.

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
asks `InstanceCommandQueue` to execute local commands. The queue does not
dequeue a command while `InstanceResponseQueue` has no capacity for its single
response. DSP state is therefore changed on the same audio thread that
subsequently calls `DspChain::Process()`.

## Threading and queue policy

`ConcurrentQueue` is mutex-based because coordinator queues are not consumed by
an audio thread. `SpscQueue` is a generic fixed-capacity
single-producer/single-consumer lock-free queue. The instance command and
response wrappers use it with these roles:

- command queue producer: the coordinator worker;
- command queue consumer: the target instance's audio callback;
- response queue producer: the target instance's audio callback;
- response queue consumer: the coordinator worker.

The audio thread takes no mutex while draining its local queue. If that queue
is full, `CommandDeliveryQueue` retains the command in a FIFO queue belonging to
that instance and retries it on the coordinator worker; later commands for the
same instance cannot overtake it and commands are not silently discarded.
Pending delivery storage is currently unbounded. Latest-value coalescing is not
enabled because replacing an older request would also require an explicit
protocol policy for its response.

Responses use one common coordinator `ConcurrentQueue`. The audio thread writes
to the instance-local `InstanceResponseQueue`; the coordinator worker drains
local responses into the common queue. `InstanceCommandQueue` applies
backpressure before executing a command, so a response is never created without
capacity to enqueue it. The current command variant therefore has the explicit
invariant that every local command produces exactly one response.

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
  -> InstanceCommandQueue on the audio owner
  -> StateCommandHandler returns a response
  -> InstanceResponseQueue
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
`StateResponsePublisher` owns numbering and publication of coordinator response
parts. `StateRouter` contains cross-instance equalizer target resolution and
bank-path rewriting. `CommandDeliveryQueue` owns ordered retry delivery to
instance command queues.

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
