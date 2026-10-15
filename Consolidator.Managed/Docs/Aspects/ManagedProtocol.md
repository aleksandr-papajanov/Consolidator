# Managed Protocol

Managed receives a selector and decoded `Atom` values from Native. Native owns
only host values to `NativeAtom` conversion. Managed owns wire validation, command
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
The protocol client registers pending request callbacks only for commands that supplied a real
callback; fire-and-forget gesture writes cannot accumulate pending entries when
their intermediate commands are coalesced. Successful callbackless gesture
writes do not emit `action_done`; rejected results and execution errors retain
their ordinary response. A write with a real callback is sent without a
coalescing transaction ID so its request always receives a response.
Registry lifecycle and label/group updates use revisioned typed delta messages.
The VST3 client applies a delta only when its previous revision matches the
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

For ordinary local/group writes, zero transaction ID denotes a standalone edit.
Managed creates a history point only when prepared changes include a registered
history value. Nonzero IDs belong to a continuous gesture's history framing.
See [StateHistory.md](StateHistory.md); UI code holds no per-parameter policy.

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

The selected-bank analyzer filter curves are calculated in VST3 presentation from
focused parameter bindings and local filter definitions. The analyzer also
requests `observe_banks`, a read-only coordinator query that returns the raw
equalizer values for every bank of the target instance in one
`bank_state_snapshots` frame. VST3 presentation calculates a separate all-banks
equalizer response by summing the active bank responses in dB at each
frequency. It is displayed alongside the existing selected-bank/filter
curves; this query does not alter target selection or editable bindings.
`fft` and `dsp_telemetry` are the streamed latest-only analysis frames; the
latter is defined in [`DspTelemetry.md`](Telemetry.md).

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
never occupy an outgoing backlog. After Managed confirms activation, the VST3
client requests one current target snapshot and resumes control bindings only
after that snapshot is complete. Inactive
`LatestAnalysis` entries currently pass through unchanged;
their classification reserves a later managed coalescing policy.

Command contracts: [TargetObservation](TargetObservation.md), [ResetCommands](ResetCommands.md),
[InstanceControls](InstanceControls.md), [ProcessorActivity](ProcessorActivity.md),
[Topology](Topology.md), [StateHistory](StateHistory.md), [Approximation](Approximation.md).
Spectrum wire payload: [SpectrumProtocol](SpectrumProtocol.md).
