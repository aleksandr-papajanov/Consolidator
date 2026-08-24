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

`ProtocolService` is the Native-facing entry point. Its flow is deliberately
direct:

```text
ProtocolInput
  -> CommandDecoder
  -> CommandEndpointRegistry
  -> CommandEndpoint<TCommand, TResult>
  -> one or more ProtocolOutput frames
  -> IProtocolTransport
```

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

The source external ID is carried by `ProtocolInput`. UI writes also carry one
explicit target instance ID outside their relative state paths, so editing a
remote row does not change the observed view merely to address a command.

## Dispatch and responses

`CommandEndpointRegistry` owns the command-type-to-endpoint map and also checks
that selectors are unique. A `CommandEndpoint<TCommand, TResult>` passes its
typed command to `InstanceCommandRouter`, encodes the aggregate
`CommandExecutionResult<TResult>` with `CommandResponseEncoder`, and addresses
the resulting frame or multipart frame sequence to the source external.

All protocol command types implement `IInstanceCommand<TResult>`. Command
definitions and results live in Core; they do not depend on Protocol.
`CommandScope` determines whether routing resolves the observed target,
connected instances or a coordinator-wide operation. An explicit UI target
takes precedence over derived target routing. Coordinator-scoped commands execute
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
`NativeOutputService` implements both `IProtocolTransport` and
`IProtocolOutputRegistry`: transport sends outputs, while the registry exposes
callback registration, removal and a snapshot of registered delivery IDs. The service owns
the `instanceId -> IProtocolOutputCallback` map and fans out to deduplicated
targets. Protocol components do not invoke callbacks directly or retain callback
data.

The command surface contains state read/write, state reset, history framing,
history jumps, UI initialization, target observation and registry snapshots.
`initialize` returns the external's managed instance ID. `observe_target`
selects an `(InstanceId, BankId)` view and returns a multipart target snapshot.
There is no UI session, epoch or selected-bank state. Later changes use
`state_changed` with the same semantic paths and range metadata. Reset writes the target
state subtree's initial values through the normal setters, so peer propagation
and observers remain authoritative. State changes are published by the existing
observer chain and addressed by `StateChangeRouter` using topology and focus.
`begin_history` and `end_history` frame a global history action. `jump_history`
moves the shared `StateHistory` cursor to a logical history
position. Every successful history advance or jump publishes a `history_state`
snapshot to all registered instances; the snapshot contains revision, cursor,
entry count and navigation availability. Registry reads current managed state
and returns a structured atom sequence. Command handlers remain in Core and do
not move routing or state mutation into codecs.
