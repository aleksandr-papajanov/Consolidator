# Processor Activity

The processor set is Gain Staging, Saturator, Shaper, Compression, Equalizer and Polish.
Polish activity is derived from
its `thick` and `air` macro values. Bypass is exposed separately so an active
device can be rendered with a disabled marker color.

Managed owns one local activity observer for the five instance-owned
processors and all seven equalizer banks. VST3 presents the published
activity values directly. The row-level activity markers beside the track name
remain part of the presentation. When a focused bank belongs to a group, each
row's marker is resolved from the equalizer device activity: it is active when
at least one bank in the instance is active, regardless of the focused bank or
group membership.

Each processor status contains `processorId`, `effectActive`, and `bypassed`.
Equalizer bank activity remains a separate bank-owned status, calculated by the
same observer. Activity describes active content independently of bypass;
bypass is carried separately for presentation.

Activity uses a small epsilon of `0.0001` to distinguish neutral from changed
control values. Gain Staging is active for non-zero input/output level, Width
different from neutral 1, either Leveler enabled, or Limit below 0 dB. Saturator
is active for non-zero drive/output; Shaper for non-zero attack/sustain/output;
Compression for non-zero reduction/output; Polish for non-zero thick/air. The equalizer definition is
unchanged: a bank is active when it is not bypassed and contains at least one
non-bypassed filter whose gain exceeds that epsilon.

Processor changes are published as one revisioned
`registry_processor_changed` delta. A registry snapshot contains all six
statuses for every instance. The VST3 registry client applies deltas only when
the previous revision matches; a gap requests a complete snapshot.

Bank and processor activity are recalculated from state changes. The observer
publishes only transitions, while registry snapshots use the same activity
definition.

Processor bypass uses an explicitly targeted command:

```text
set_processor_bypass targetInstanceId processorId local|group 0|1
set_bank_bypass targetInstanceId bankIndex local|group 0|1
```

Managed resolves the target instance directly and uses the source instance's
focused bank only to resolve the corresponding group scope. Bank bypass uses
the explicitly supplied zero-based bank index. Group targets are resolved by
`InstanceControlTargetResolver`. An ungrouped bank resolves to the addressed
instance/bank alone. Processor and bank bypass transactions do not create
history points or change UI selection. Direct state writes to
instance-owned processor bypass values are rejected. Processor controls expose
only bypass and reset operations.


## Viewer-specific markers

Each `registry_processor` snapshot frame additionally carries the
viewer-specific `markerActive` value after `effectActive`. Managed recalculates
that value from exact topology and focused-bank changes and publishes targeted
`registry_processor_markers_changed` batches containing the changed processors
grouped by instance. Marker frames do not advance the global registry revision because the
projection can differ between viewers at the same revision.

Reset commands: [ResetCommands](ResetCommands.md).
Frontend hit testing and marker presentation: [TrackInteraction](../../../Consolidator.Vst3/Docs/Aspects/TrackInteraction.md).
