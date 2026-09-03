# Managed Protocol

Managed receives a selector and decoded `Atom` values from Native. Native owns
only `Max atom -> NativeAtom` conversion. Managed owns wire validation, command
construction, relative state-path decoding, routing and output encoding.

Dependencies remain one-way:

```text
Native
  -> Protocol
      -> Routing
          -> Core
      -> State contracts

Core
  -> State infrastructure
```

Protocol owns the wire boundary. Routing selects command and notification
targets. Core owns command definitions, concrete state behavior and lifecycle.
The top-level State scope supplies paths and tree nodes without depending on any
of those consumers. Read and write handlers perform their runtime-typed tree
access directly; there is no general state-access service.

## Folder structure

`Consolidator.Managed/Protocol` is organized by the message pipeline:

```text
Protocol/
├─ Messages/       wire values and input/output envelopes
├─ Decoding/       selector codecs, headers and state-path decoding
├─ Dispatch/       decoded commands and typed command endpoints
├─ Encoding/       response, error, state-change and atom encoding
├─ Notifications/  state-change publication
├─ Transport/      outbound managed transport contract
└─ ProtocolService.cs
```

`ProtocolService` is the Native-facing entry point. It decodes on the calling
thread, then enqueues a bounded command for one managed control worker:

```text
ProtocolInput
  -> CommandDecoder
  -> bounded FIFO control queue
  -> CommandEndpointRegistry (single worker)
  -> CommandEndpoint<TCommand, TResult>
  -> one or more ProtocolOutput frames
  -> IProtocolTransport
```

There is no synchronous execution path from the Native ABI entrypoint. Pending
writes with the same source, scope, ordered path shape, and transaction are
coalesced at their existing FIFO position, preserving the final gesture values
for both single-value and multi-value controls. The latest-value slot and the
presence of its FIFO placeholder change under one short control-path lock, so a
producer cannot publish a replacement between worker removal and placeholder
ownership and leave that final value without queued work.
Max registers pending request callbacks only for commands that supplied a real
callback; fire-and-forget gesture writes cannot accumulate pending entries when
their intermediate commands are coalesced. Successful callbackless gesture
writes do not emit `action_done`; rejected results and execution errors retain
their ordinary response. A write with a real callback is sent without a
coalescing transaction ID so its request always receives a response.
Registry lifecycle and label/group updates use revisioned typed delta messages.
The Max client applies a delta only when its previous revision matches the
current snapshot; a gap triggers one full snapshot resynchronization.
There is no general message hierarchy or combined codec registry. The protocol
has one concrete input envelope, one concrete output envelope, typed input
codecs and explicit encoders for each output kind.

## Input decoding

`CommandDecoder` owns the selector-to-`IInputCodec` map. Each registered input
codec validates one selector and creates a typed Core command. Duplicate
selectors fail during service construction.

`StatePathDecoder` converts relative atom paths into `StatePath` values.
`WriteInputCodec` additionally decodes and validates the value for the target
path. Decoders do not select targets or mutate state.

The source external ID is carried by `ProtocolInput`. Ordinary UI writes do not
carry a target instance or bank ID. Managed resolves both from the source
instance's `SelectionContext`. Their request bodies begin with a propagation
scope:

```text
write local|group transactionId entryCount entries...
write topology targetInstanceId transactionId entryCount entries...
```

Each entry is encoded as `entry path... value valueAtoms mode`, where `mode`
is the symbol `copy` for an absolute value or `delta` for a relative float
change. The mode belongs to the write operation, not to the state definition.

`topology` is a separate explicit-address contract used only for bank-group
membership changes over a user-selected bank set.

The `clear_topology` command is a coordinator-scoped topology operation. It
clears every non-default bank group (`groupId > 0`) in Managed state and keeps
the default group `0` unchanged.

## Dispatch and responses

`CommandEndpointRegistry` owns the command-type-to-endpoint map and also checks
that selectors are unique. A `CommandEndpoint<TCommand, TResult>` passes its
typed command to `InstanceCommandRouter`, encodes the aggregate
`CommandExecutionResult<TResult>` with `CommandResponseEncoder`, and addresses
the resulting frame or multipart frame sequence to the source external.

All protocol command types implement `IInstanceCommand<TResult>`. Command
definitions and results live in Core; they do not depend on Protocol.
`CommandScope` determines whether routing resolves the selected target or a
coordinator-wide operation. Explicit targets are limited to target observation,
diagnostic/query commands, and topology membership writes. Coordinator-scoped commands execute
once through the source instance command gate, while their state operation is
shared globally by the injected coordinator service.

## Notifications

Committed state changes leave Core through `IStateChangeSink`.
`StateChangePublisher` asks `StateChangeRouter` for target instance IDs, encodes
the change with `StateChangeEncoder`, and sends the result through
`IProtocolTransport`.

Instance-owned changes are sent to every external observing that instance.
Bank-owned changes are sent to externals observing the changed bank. Routing derives the
bank address from `(InstanceId, StatePath)`; state-change events do not carry a
second bank address.

Notification delivery failures do not turn an already committed state mutation
into a failed operation.

Analyzer curves are calculated entirely in JavaScript from the focused
parameter bindings and their local filter definitions. Managed publishes no
curve arrays, raw equalizer projections, filter catalogs, or analyzer
configuration frames. `fft` remains the only streamed analysis frame.

## Routing and Core ownership

`Consolidator.Managed/Routing` contains only target selection:

```text
Routing/
├─ Commands/       InstanceCommandRouter and routing result
└─ Notifications/  StateChangeRouter
```

The derived `TopologyIndex` lives in the shared `Core/Topology` scope. Routing
consumes its queries but does not own the topology implementation.

`InstanceRegistry` lives in `Core.Services.Instances` because it owns lifecycle
and per-instance state composition. `CommandExecutor` lives in
`Core.Commands.Execution` because it executes commands on already-resolved
targets. Neither responsibility belongs to Routing.

`InstanceCommandRouter` is the single command-routing boundary. It validates
the source instance, resolves targets through `TopologyIndex`, serializes the
complete routed operation with `IOperationGate`, and delegates execution to
`CommandExecutor`. Command handlers and the services they invoke execute inside
that gate and must not enter it again. The executor never chooses targets.

## Output boundary

`ProtocolOutput` carries `TargetInstanceIds`, a selector and atoms.
It also carries explicit delivery semantics: `Lossless` for responses, errors,
lifecycle and history command outputs; `ActivePresentation` for state
notifications and analyzer configuration; and `LatestAnalysis` for FFT frames. Transport behavior
is selected by this contract, never by selector string.
`NativeOutputService` implements both `IProtocolTransport` and
`IProtocolOutputRegistry`: transport sends outputs, while the registry exposes
callback registration, removal and a snapshot of registered delivery IDs. The service owns
the `instanceId -> IProtocolOutputCallback` map and fans out to deduplicated
targets. Protocol components do not invoke callbacks directly or retain callback
data.

Protocol responses use `IProtocolTransport` directly. Presentation publishers use
`IPresentationTransport`, implemented by `PresentationOutputGate`, which tracks
active state per registered instance. Active instances receive presentation
entries immediately; inactive `ActivePresentation` entries are discarded and
never occupy an outgoing backlog. After Managed confirms activation, the Max
client requests one current target snapshot and resumes control bindings only
after that snapshot is complete. Inactive
`LatestAnalysis` entries currently pass through unchanged;
their classification reserves a later managed coalescing policy.

The command surface contains state read/write, state reset, history framing,
history jumps, UI initialization, target observation, instance activity and
registry snapshots.
Instance controls use the same target resolution for mute and solo. A grouped
bank resolves to all members, while an ungrouped bank resolves to its single
instance. In the bank manager, `S` is exclusive on a regular click and becomes
additive with Shift-click; `M` and `B` are always additive, so regular clicks
can accumulate mute or bypass across instances. If the clicked instance's
focused bank belongs to a group, additive operations keep group scope and add
or remove the complete group. A non-additive solo gesture keeps the current
scope; an ungrouped instance is treated as a single-instance group. Selecting
another grouped bank is the explicit way to solo a different group.
`initialize` returns the external's managed instance ID and transient UI context.
The default context is `equalizer`. `observe_target` receives
`targetInstanceId`, `bankId` and one strict context symbol (`input`, `saturator`,
`compressor`, `equalizer`, `polish` or `output`), then atomically records the UI context
and target before returning one `target_state_snapshot` frame containing the
target identity, bank, context, entry count, and each relative path
with its value and physical/effective ranges. A successful frame implies
`ready` for every entry; an error applies to the complete frame.
The context is per-instance transient UI state: it is not a `StateValue`, does
not participate in history or persistence, and never affects DSP. Later changes use
`state_changed` with the same semantic paths and range metadata. Reset writes the target
state subtree's initial values through one prepared transaction, so peer
propagation remains authoritative and observers see only the complete reset.
The reset frame carries an explicit `local`, `group`, or `group_instance` scope. Local reset
prepares only the addressed state values. Group reset prepares every resolved
peer with that peer's own initial value; it never applies a delta from the
source value. `group_instance` resolves the exact group of the selected bank and
resets the complete DSP tree of each member instance, including every equalizer
bank.
Its body is:

```text
reset transactionId local|group|group_instance statePath...
```

Targeted reset uses the same `reset` selector and addresses the target before
the transaction fields:

```text
reset target targetInstanceId bankIndex|none transactionId local|group|group_instance statePath...
```

`bankIndex` is required for an equalizer bank reset and is `none` for a
processor reset. Targeted reset never changes selection and can address an
instance other than the source external. The Bank Manager uses targeted reset
only for Ctrl/Command-double-click; an ordinary double-click does not reset.

Processor-panel `R` uses `local` for a regular click and `group` for a
Ctrl/Command-click. Managed resolves the focused-bank context for the source
external and uses it during propagation.
The reset selector uses the same relative path grammar as state writes. A leaf
resets one value; `equalizer.filter.N` resets a complete filter;
`equalizer.bank` resets the focused equalizer bank; a processor path such as
`compressor` resets the complete processor, and `dsp` resets all DSP settings.
The Max UI emits one `reset` intent for a double-click and never supplies the
default value itself.
Target selection suspends bindings before sending
`observe_target`, then replaces the target cache and resumes after the complete
snapshot. Instance activation keeps bindings inactive until this same target
transition completes.
State changes are published by the existing
observer chain and addressed by `StateChangeRouter` using topology and focus.
State writes pass through registered path-scoped policies before the common
typed write validation and history transaction. Bank-group writes are checked
by the bank-group policy: automatic group `0` is immutable, a bank cannot be
assigned to a second group, and one track cannot contribute multiple banks to
the same group. Rejected policy writes do not open or commit a history
transaction.
`begin_history` and `end_history` frame a global history action. `jump_history`
moves the shared `StateHistory` cursor to a logical history
position. Every successful history advance or jump publishes a `history_state`
snapshot to all registered instances; the snapshot contains revision, cursor,
entry count and navigation availability. UI initialization also publishes the
current snapshot so a newly connected Max UI can render existing history before
the next history mutation. Registry reads current managed state
and returns a structured atom sequence. Each registry instance entry carries
its instance-level `mute` and `solo` values. Each registry bank entry also carries
`effectActive`, which is true when the bank is not bypassed and has an active
filter with non-zero gain. Each bank entry also carries its raw `bypassed`
value. Managed emits `registry_bank_effect_changed` and
`registry_bank_bypass_changed` when the corresponding bank status changes;
instance mute and solo changes use their corresponding registry deltas. Instance controls use dedicated
`set_instance_mute`, `set_instance_solo`, and `set_instance_bypass` commands;
direct protocol writes to the instance `mute`, `solo`, and `bypass` paths are
rejected. Each command carries an
 explicit target instance ID, `local` or `group` scope, and `exclusive` or
`additive` selection mode. Bank bypass controls carry an explicit bank index.
Managed resolves that target through `ContextualBankResolver`. Group resolution never
traverses another group on the same track and falls back to the target instance
when the bank is ungrouped. Both handlers update the resolved
local instance values in one atomic state transaction without creating a
history point. Later topology changes do not migrate the stored mute or solo
values. Command handlers remain in Core and do not move routing or state
mutation into codecs.

After the common command-frame header, the request bodies are:

```text
set_instance_mute instanceId local|group 0|1 exclusive|additive
set_instance_solo instanceId local|group 0|1 exclusive|additive
set_instance_bypass instanceId local|group 0|1 exclusive|additive
```

Each registry instance also carries five instance-owned processor statuses.
Managed derives `effectActive` from processor state and publishes
`registry_processor_changed` only when the complete status changes. Each
`registry_processor` snapshot frame additionally carries the
viewer-specific `markerActive` value after `effectActive`. Managed recalculates
that value from exact topology and focused-bank changes and publishes targeted
`registry_processor_markers_changed` batches containing the changed processors
grouped by instance. Marker frames do not advance the global registry revision because the
projection can differ between viewers at the same revision. Processor bypass
uses the dedicated `set_processor_bypass` command. Bank bypass uses the
dedicated `set_bank_bypass` command. Both commands carry an explicit target
instance and do not change the source UI selection. Direct writes to
instance-owned processor bypass values are rejected. The full contract is
documented in [ProcessorActivity.md](ProcessorActivity.md).

The request bodies are:

```text
set_processor_bypass targetInstanceId processorId local|group 0|1
set_bank_bypass targetInstanceId bankIndex local|group 0|1
```

`bankIndex` is zero-based and must be in the range `0..6`. The target bank group
is resolved for the explicit target instance at that index; a missing target or
bank context is rejected without falling back to the source selection.

Where a `bankId` is part of an observation, topology, registry, or notification
contract, it is zero-based and uses the range `0..6`. The protocol,
JavaScript clients, codecs, registry messages, state paths, and internal
`BankId` all use this same value without index conversion.
