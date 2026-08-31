# Persistence

Managed state is the authoritative persistence source. Each Native external
exposes one hidden, store-only Max parameter named `value`; JavaScript does not
serialize state and does not participate in save or restore.

## Save path

```text
committed Managed value change
  -> persistence_dirty callback
  -> Native atomic coalescing
  -> Max qelem
  -> object_parameter_value_changed(blobnotify = 1)
  -> value/getvalueof
  -> Managed control-queue barrier
  -> versioned UTF-8 JSON snapshot
  -> compact numeric Max Blob value
```

The control-queue barrier places capture after every command already accepted
by the bounded FIFO. Capture therefore observes one committed state boundary,
not an intermediate UI or DSP projection. Managed allocates the returned ABI
buffer and Native releases it with `ConsolidatorFreePersistence` after copying
the bytes into the Max value object.

The Min `value` attribute is a numeric array used as a generic Max Blob. The
first two atoms contain a format marker and UTF-8 byte length. Remaining atoms
pack six bytes each into exact integer-valued doubles. Six bytes stay within
the 53-bit exact integer range of IEEE 754, so Max may store or textualize the
array without losing payload bits. No snapshot text enters Max's process-global
symbol table, and the temporary atom arrays are reclaimed normally.

Payloads are limited to 380 KiB, which keeps the packed representation below
Max's `0xFFFF` array-attribute atom limit. The current schema is substantially
smaller; the bound also prevents a malformed saved value from forcing an
unbounded Native allocation. Decode rejects the wrong marker, non-integral
atoms, inconsistent lengths, extra/truncated chunks, hidden trailing bytes and
out-of-range packed values before calling Managed restore.

The current payload is schema `2`. It stores input `level`, `target`, `width`,
and `leveler`; saturation `drive`, `curve`, `split`, and `output`; compressor
`attack`, `sustain`, `compression`, enum `character`, `parallel`, and `output`;
Polish `thick` and `air`; and output `level`, `target`, and `limiter`. The
previous threshold/ratio/release/mix model is not decoded as a compatibility
format.

The numeric Blob marker is the only supported Max-side representation. Saved
development devices created with the former JSON-symbol representation must be
saved again; Native does not keep a parallel legacy decoder.

## Restore path

```text
Max value/setvalueof
  -> validate and unpack numeric atoms
  -> copy UTF-8 bytes into Managed
  -> Managed control-queue barrier
  -> strict schema validation
  -> one StateHistoryTransaction
  -> replace every history slot
  -> publish the complete DSP snapshot
```

Validation completes before state storage is committed. Unknown schema
versions, unknown JSON properties, missing required constructor properties,
invalid bank topology, invalid array shapes and non-finite or out-of-range DSP
values reject the entire payload.

Restore fills all history slots, including when the current slot already equals
the restored value. Undo or Redo after loading therefore cannot expose state
from before the restore. Observer notifications are emitted only for effective
current-value changes. Persistence dirty notifications are suppressed for the
restored instance while the baseline transaction runs.

## Serialized state

Schema version 1 contains:

- instance mute and solo;
- group membership for all seven banks;
- input and output gain;
- saturator and detector state;
- compressor and detector state;
- input and detector state;
- equalizer bypass/solo and all filters in all seven banks.

Instance IDs, labels, UI selection, analyzer state, runtime handles, DSP exchange
buffers and history cursor metadata are not serialized.

## Max parameter lifecycle

- `class_parameter_init` runs from Min's `maxclass_setup`, after attributes exist.
- Each object calls `object_parameter_init_flags` with `PARAM_TYPE_BLOB` and
  `PARAM_FLAGS_FORCE_TYPE`.
- `parameter_enable` is `3` (`blobonly`) and `parameter_visibility` is `1`
  (`store-only`).
- Native calls `object_parameter_value_changed` only from its Max-thread qelem.
- A setter callback re-entered synchronously by that local change notification is
  ignored. Only a setter invocation outside the notification restores the Managed
  baseline, so publishing a new Blob cannot replace the active history timeline.
- Object destruction releases the Max parameter after unregistering Managed
  callbacks.

Persistence capture and restore are control-path operations and never run from
the audio callback.

## Verification

Managed tests cover FIFO capture ordering, strict rejection without mutation,
history-slot replacement, dirty-notification suppression during restore and DSP
publication for indirectly affected instances. NativeAOT integration verifies
the three exports, caller-owned buffer release and a capture/restore round trip.
Native component tests verify the numeric Blob codec, malformed input rejection
and the Max-compatible size bound.
The actual Live Set dirty/save/load behavior still requires the Max/Ableton host
pass.
