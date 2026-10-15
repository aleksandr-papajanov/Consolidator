# Persistence

Managed state is the authoritative persistence source. VST3 host callbacks
capture and restore the committed Managed UTF-8 snapshot; ViewModels/bindings
never serialize state. Host lifecycle and dirty notification delivery are in
[VST3 Host](../../../Consolidator.Vst3/Docs/Aspects/Host.md).

## Save

```text
committed Managed value change → persistence_dirty callback
→ native atomic coalescing → JUCE message-thread host dirty notification
host getStateInformation → Managed control-queue barrier
→ versioned UTF-8 JSON snapshot → VST3 state bytes
```

The FIFO barrier captures after every already accepted command, at one committed
state boundary rather than an intermediate UI/DSP projection. Managed allocates
the returned ABI buffer; the caller copies it and releases it exactly once with
ConsolidatorFreePersistence. Presentation can be closed throughout save/restore.
Persistence operations never run in the audio callback.

## Restore

```text
host setStateInformation → copy borrowed UTF-8 bytes
→ Managed control-queue barrier → strict validation
→ one StateHistoryTransaction → replace all history slots
→ observers / affected DSP snapshot publication / open-view refresh
```

Validation completes before storage commit. Unknown schema versions/properties,
missing required constructor properties, invalid topology/array shapes and
non-finite or out-of-range values reject the whole payload without partial state.
Restore fills every history slot even when the current value already matches:
Undo/Redo cannot expose pre-load values. Observers receive only effective active
value changes; dirty notifications are suppressed for the restored instance
while its baseline transaction runs.

## Schema 8

- Instance mute, solo and bypass; group membership for all seven banks.
- Unified Gain Staging: input/output Level, Target, Leveler, Limit, Width,
  shared Bypass and input-detector filters.
- Saturator values, bypass and detector filters.
- Independent Shaper and Compression values/bypasses; Compression type profiles,
  parallel mix and detector filters. Active values must match the selected profile.
- Polish values/bypass; EQ processor/bank bypass and every filter in every bank.

Instance IDs, host labels, UI selection/zoom, analyzer presentation, runtime
handles/buffers and history cursor metadata are not serialized. Old schemas and
separate Input/Output objects are rejected without compatibility decoder.
Defaults/paths: [GainStaging](GainStaging.md). The host stores raw snapshot bytes;
there is no numeric atom Blob wrapper or duplicate XML state tree in this path.

## Verification scope

Managed use cases cover FIFO capture order, strict rejection, history baseline,
restore dirty suppression and publication for indirectly affected instances.
NativeAOT integration covers the three exports, correct buffer release and an
actual capture/restore round trip. The installed VST3 still requires an authorized
Live dirty/save/load/reopen pass; component results do not prove DAW behavior.
