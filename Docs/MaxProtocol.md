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
read        := read version source wireId count (query path)*
write       := write version source wireId count (entry path valueMarker valueAtom)*
reset       := reset version source wireId path

version     := the integer `1`
source      := a non-empty Max symbol identifying the client endpoint
wireId      := an unsigned decimal symbol in `0..9007199254740991`
count       := an integer in 0..16

query       := the symbol "query"
entry       := the symbol "entry"
valueMarker := the symbol "value"

path        := semantic-path
semantic-path := selected_bank | mute | solo | bank bankNumber |
                bank bankNumber group |
                device [detector [filter filterNumber]] (parameter | marker)
bankNumber  := an integer in 1..7
filterNumber := an integer in 1..7 (1..2 for detector filters)
device      := main_input_gain | main_output_gain | saturator | compressor | equalizer
parameter   := gain | frequency | q | drive | mix | detector_amount |
               threshold | ratio | attack | release
marker      := bypass | solo | listen
valueAtom   := a path-typed Max atom
```

The current protocol version is `1`. It is mandatory after every input
selector and before `source`; any other version produces
`unsupported_version`. Older input formats are not accepted and there is no
compatibility fallback.

The parser must consume exactly the list. Extra or missing atoms, an invalid
count, or duplicate path components are protocol errors. The command is
implicitly addressed to the adapter-bound `ConsolidatorInstance`; clients do
not need its process-local Core instance ID before their first request. `read`
uses `count` `query` sections; `write` uses `count` `entry` sections. The
`query`, `entry`, and `value` markers delimit variable-length semantic paths;
`AtomPathCodec` never guesses a path boundary from the remaining atoms. `reset`
uses one complete target path and never carries a value.

For `read`, `count 0` is a valid full-instance snapshot request and therefore
has no `query` section: `read 1 source request 0`. A zero-count `write` is also
valid and has no `entry` section.

Examples:

```text
read 1 ui <symbol 10> 2 query compressor query equalizer
write 1 ui <symbol 11> 2 entry compressor threshold value -18. entry compressor ratio value 4.
```

The angle-bracket notation above means a Max symbol, not a literal message-box
token. In a Max patch, create these wire IDs with `[sprintf %s 10]` or
`[sprintf %s 11]` and connect the result to the command builder. A plain
`10` or `11` typed directly into a message box is an integer atom and is not a
valid wire ID.

Paths are semantic and variable-length. Examples are `compressor`,
`compressor threshold`, `compressor detector`,
`equalizer bank 2`, `equalizer bank 2 filter 4`,
`equalizer bank 2 filter 4 gain`, `bank 2 group`, `selected_bank`, and
`compressor detector listen`. Every completed prefix is valid for `read`; a
`write` additionally requires a concrete path accepted by `AtomValueCodec` and
Core. `AtomPathCodec` is the only component that knows how these forms map to
the current Core `StatePath`; the internal field, optional IDs, node depth, and
node array are not part of the wire protocol.

`bank N ...` is reserved for topology and currently only supports
`bank N group`. DSP paths always use the canonical `equalizer bank N ...`
form, including bank-level markers such as `equalizer bank 2 bypass`.

Values are decoded by `AtomValueCodec` from the semantic path. `selected_bank`
accepts `bank1..bank7` (or the corresponding public number `1..7`) and produces
a Core `BankId`. `bank N group` accepts a non-negative group number or `none`,
where `none` produces an empty group. DSP parameters accept integer or
floating-point Max atoms and produce Core `float` values. Markers, mute, and
solo use strict integer values `0` or `1`; symbolic `true` and `false` are
invalid.

The value codec is path-directed. In particular, `true/false`, `bank1..bank7`,
`none` for `group_id`, signed `int32`, and floating-point DSP values are not
distinguished by a generic integer decoder.

## Response frames

The adapter emits a terminal response sequence for every accepted command. The
input selectors are `read`, `write`, and `reset`; `state_begin`, `state_entry`,
`state_done`, `action_done`, and `error` are output framing selectors. Every
output frame carries version `1` immediately after its selector:

```text
action_done 1 source request instance status
error 1 source request instance code message
```

`action_done` is the terminal frame for an action command. Its `status` is `accepted`
or `rejected`; reset uses `accepted` only after realtime queue admission.
`error` is reserved for adapter/protocol failures before a Core command is
accepted. Its `code` is one of `malformed`, `unknown_source`,
`duplicate_request`, `unknown_instance`, `invalid_path`, `invalid_value`, or
`batch_overflow`. `message` is diagnostic text and is not used for routing.

State responses are multipart at the transport boundary. Core still produces
one logical `StateResponse`; the adapter emits:

```text
state_begin 1 source request instance truncated entryCount
state_entry 1 source request instance index path value writeStatus physicalMinimum physicalMaximum minimum maximum
state_done  1 source request instance
```

`request` and `instance` are correlation/instance IDs encoded as decimal
symbols, matching the input `wireId` contract. Structural numeric fields such
as `count`, `index`, bank/filter numbers, ranges, and `GroupId` values remain
integer or floating-point atoms as specified by their field. `state_begin`
is emitted once, followed by `entryCount` `state_entry` frames,
then `state_done`. `index` is zero-based and contiguous. `state_begin` and
`state_done` use the response envelope instance. Each `state_entry` uses the
entry path's target instance, falling back to the response instance when the
path has no instance. This preserves grouped responses that target multiple
instances. The three frames repeat `(source, request, instance)` so clients can
demultiplex interleaved responses. `state_done` is terminal for read/write;
`action_done` is terminal for an action; `error` is terminal for adapter/protocol
failures.

`state_begin` is retained because it carries the snapshot metadata
`truncated` and `entryCount`; consumers can use it to allocate or validate the
following entry sequence before receiving `state_done`.

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
whether a value is a boolean marker, `BankId`, `GroupId`, or a DSP parameter
value; atom type alone is insufficient.

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
delivers responses dequeued from that instance through `controlOutput`. Core
exposes a response notifier callback; the external binds it to a thread-safe
`queue<>.set()` signal. The callback only schedules a Max main-thread drain: a
thread-safe scheduling call is permitted, but it does not call outlets, emit
messages, or encode atoms. Analysis uses a separate `analysisOutput` outlet.
The UI sends `analysis_view <instanceId> <bank>` to any physical external to
select the global analysis view; `bank` uses the public `1..7` numbering. The
UI then sends argument-free `analysis_tick` to read changed persistent
snapshots on the Max main thread and emit selector-framed telemetry;
analysis has no external worker, notification callback, or response queue.
Telemetry selectors are `meter <point> <rmsDb> <peakDb> <smoothedDb>`,
`saturator_distortion <percent> <smoothedPercent>` (normalized nonlinear
residual percent, not spectral THD), and
`compressor_reduction <rmsDb> <peakDb> <smoothedDb>`. The meter points are
`input_gain`, `saturator`, `compressor`, and `output_gain`. Compressor reduction
values are positive reduction dB, where `0` means no attenuation. Level
`smoothedDb` values are produced by smoothing linear RMS before dB conversion.
