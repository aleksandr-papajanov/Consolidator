# Max protocol boundary

This document fixes the public atom protocol before codec and adapter
implementation. The protocol is a transport contract; Core receives only
typed commands and never sees Max sources or atoms.

## Correlation

Every wire request is identified by the pair `(source, requestId)`. `source`
identifies the Max client/endpoint and `requestId` is scoped to that source.
Wire IDs are decimal symbols, not Max integer atoms. They are canonical
unsigned decimal values in `0..9007199254740991` (`2^53-1`), so JavaScript
clients may retain them exactly as strings. Core `RequestId` and `InstanceId`
remain `uint64_t`; an ID outside this wire range is an adapter error.
The stateful `MaxProtocolAdapter` allocates a monotonically increasing Core
`RequestId` for every accepted wire request and stores:

```text
(source, wire requestId) -> Core RequestId
```

The mapping remains until the terminal response is encoded. Responses are
returned with the original `(source, wire requestId)` pair. The source is not
added to Core command or response types.

Every accepted state command produces one terminal `StateResponse`. Every
accepted action command produces one terminal `ActionResponse`. For
`ResetDspCommand`, `ActionStatus::Accepted` means that the reset event was
placed in the instance realtime queue; it does not mean that DSP execution has
completed.

## Framing and grammar

One inbound Max list is one complete command frame. Outbound state responses
may use the multipart frames defined below. There is no implicit end-of-batch
marker. The following grammar uses `*` for repetition and `none` for an
explicit absent value:

```text
command     := read | write | reset
read        := state read  source request count path*
write       := state write source request count (path value)*
reset       := action reset source request path

state       := the symbol "state"
action      := the symbol "action"
source      := a non-empty Max symbol identifying the client endpoint
count       := an integer in 0..16

path        := field instance device parameter marker nodeCount node*
nodeCount   := an integer in 0..3
node        := detector | bank1..bank7 | filter1..filter7
value       := a path-typed Max atom
```

The parser must consume exactly the list. Extra or missing atoms, an invalid
count, or duplicate path components are protocol errors. The command is
implicitly addressed to the adapter-bound `ConsolidatorInstance`; clients do
not need its process-local Core instance ID before their first request. `read`
uses `count` paths; `write` uses `count` path/value pairs. `reset` uses one
complete target path and never carries a value.

Inbound `path.instance` must always be `none`; the adapter canonicalizes it to
the bound local instance. `field` is one of `instance_id`, `selected_bank`, `bank_id`, `group_id`,
`dsp_parameter`, `dsp_marker`, `mute`, or `solo`. The unused path components are
encoded as `none`. `parameter` uses the names listed below, and `marker` uses
`bypass`, `solo`, or `listen`. The `instance` component is `none` on inbound
commands. `device` is one of `main_input_gain`,
`main_output_gain`, `saturator`, `compressor`, or `equalizer`.

The value codec is path-directed. In particular, `true/false`, `bank1..bank7`,
`none` for `group_id`, signed `int32`, and floating-point DSP values are not
distinguished by a generic integer decoder.

## Response frames

The adapter emits a terminal response sequence for every accepted command:

```text
done  source request instance status
error source request instance code message
```

`done` is the terminal frame for an action command. Its `status` is `accepted`
or `rejected`; reset uses `accepted` only after realtime queue admission.
`error` is reserved for adapter/protocol failures before a Core command is
accepted. Its `code` is one of `malformed`, `unknown_source`,
`duplicate_request`, `unknown_instance`, `invalid_path`, `invalid_value`, or
`batch_overflow`. `message` is diagnostic text and is not used for routing.

State responses are multipart at the transport boundary. Core still produces
one logical `StateResponse`; the adapter emits:

```text
state_begin source request instance truncated entryCount
state_entry source request instance index path value writeStatus physicalMinimum physicalMaximum minimum maximum
state_done  source request instance
```

`state_begin` is emitted once, followed by `entryCount` `state_entry` frames,
then `state_done`. `index` is zero-based and contiguous. The three frames
repeat `(source, request, instance)` so clients can demultiplex interleaved
responses. `state_done` is terminal for read/write; `done` is terminal for an
action; `error` is terminal for adapter/protocol failures.

`truncated` is `0|1` and `entryCount` is in `0..512`. Every entry is:

```text
path value writeStatus physicalMinimum physicalMaximum minimum maximum
```

`writeStatus` is one of `none`, `not_handled`, `applied`, `unchanged`, or
`rejected`. Each range atom is either `none` or a path-compatible numeric
value. Thus physical and effective ranges are both preserved; `none` means the
range is not present. Read entries use `none` for `writeStatus`.

The response `instance` is always the Core `StateResponse::instanceId` or
`ActionResponse::instanceId`; it is never inferred from `source`.

An outstanding `(source, request)` pair is unique. A second request with the
same pair before the first reaches a terminal frame is rejected with
`duplicate_request`; the adapter keeps both forward and reverse correlation
indexes until terminal encoding completes.

## Public paths

The path codec uses named fields and names, not the numeric values of Core
enums. Public numbering is deliberately one-based:

- banks are `1..7` and map to Core `BankId::Bank0..Bank6`;
- EQ filters are `1..7` and map to Core `Filter1..Filter7`;
- detector filters are `1..2`.

The public parameter names are:

```text
gain, frequency, q,
drive, mix, detector_amount,
threshold, ratio, attack, release
```

`type` is not a public parameter name. It was an internal historical name for
the saturator detector amount. `output_gain` is not a separate parameter;
gain is device-scoped and covers the output gain state where applicable.

Markers are not parameters. Their public names are:

```text
bypass, solo, listen, mute
```

DSP markers decode to `StateField::DspMarker` plus `StateMarkerId`. Instance
`mute` and instance `solo` retain their dedicated instance state fields.

Group absence is represented explicitly by the public name `none`; it decodes
to an empty group (`std::nullopt`), not to a magic numeric group ID.

The codec must validate path kind before decoding a value. A path identifies
whether a value is a boolean marker, `BankId`, `GroupId`, `int32_t`, or a DSP
parameter value; atom type alone is insufficient.

## Batches

The current protocol accepts at most 16 entries in one read or write command.
An incoming batch with more than 16 entries is rejected as a whole before a
Core command is enqueued.

Read requests use the existing broad snapshot semantics: a path may be a
prefix query and the response contains all matching entries up to the response
capacity.

Write batches are intentionally not transactional. Entries are applied
independently in order. A rejected entry does not roll back earlier applied
entries, and the response reports each entry's status. Preset restore is a
separate future protocol operation and is not implied by batch write.

## Layer boundary

The protocol layer is split into:

```text
AtomPathCodec
AtomValueCodec
AtomCommandDecoder
AtomResponseEncoder
MaxProtocolAdapter (framing + correlation)
```

`ConsolidatorExternal` remains a thin Min lifecycle and port wrapper. It owns
one `ConsolidatorInstance`, forwards audio, enqueues decoded commands, and
delivers only responses dequeued from that instance. Core exposes a response
notifier callback; the external binds it to a thread-safe `queue<>.set()`
signal. The callback only schedules a Max main-thread drain: a thread-safe
scheduling call is permitted, but it does not call outlets, emit messages, or
encode atoms. It contains no state,
group, solo, constraint, or compressor logic.
