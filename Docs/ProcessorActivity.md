# Processor Activity

The processor set now includes the Polish device in addition to input,
saturator, compressor, equalizer, and output. Polish activity is derived from
its `thick` and `air` macro values together with its bypass state; it supports
the same bypass and solo processor actions as the other effect devices.

Managed owns one local activity observer for the six instance-owned
processors and all seven equalizer banks. A separate Managed processor-marker
projection combines those raw statuses with the exact topology and each
viewer's focused bank. JavaScript only presents the published values.

Each processor status contains `processorId`, `effectActive`, `bypassed`, and
`soloed`. Input and output do not support solo; their `soloed` value is always
false and their solo control is disabled. Equalizer bank activity remains a
separate bank-owned status, calculated by the same observer.

Activity uses one gain threshold of `0.5` for all gain-based decisions. A bank
is active when it is not bypassed and contains at least one non-bypassed filter
whose gain exceeds that threshold. The equalizer processor is active when the
device is not bypassed and at least one bank is active.

Processor changes are published as one revisioned
`registry_processor_changed` delta. A registry snapshot contains all six
statuses for every instance. The Max registry client applies deltas only when
the previous revision matches; a gap requests a complete snapshot.

Bank and processor activity are recalculated from state changes. The observer
publishes only transitions, while registry snapshots use the same activity
definition.

The six device markers in each registry row are per-instance values and render
the processor `effectActive` status directly. The equalizer row marker is active
when the instance equalizer is not bypassed and at least one of its banks is
active, independently of the viewer's focused bank.

The panel-navigation device markers are contextual presentation values. With a focused
ungrouped bank, the equalizer marker uses that bank's activity. With a focused
grouped bank, each device marker is active when at least one group member is
active; the equalizer checks the exact member bank address and the member
equalizer bypass. Group members can use different bank IDs. Without a focused
bank, markers use the instance processor statuses.

The projection subscribes to topology, viewer-focus, bank-activity and
processor-activity transitions. A viewer-specific registry snapshot carries
`markerActive` with every processor. Later marker transitions use one targeted,
non-revisioned `registry_processor_markers_changed` batch per viewer:

```text
registry_processor_markers_changed 1 instanceCount
    (instanceId processorCount (processorId 0|1)*)*
```

The global registry remains revisioned because labels, topology and raw
activity are shared. Marker messages are separate because two viewers may
focus different banks at the same registry revision. Snapshot delivery restores
the complete marker state when a viewer subscribes again. The Max registry
client stores both delivered values without deriving topology: registry rows
render `effectActive`, while panel-selection markers render `markerActive`.

Processor bypass and solo use source-relative commands:

```text
set_processor_bypass processorId local|group 0|1
set_processor_solo processorId local|group 0|1 exclusive|additive
```

Managed resolves the selected target and bank from the source instance's
`SelectionContext`; rendered row, instance, and bank IDs are not command
addresses. Group targets are resolved by `InstanceControlTargetResolver`. An
ungrouped bank is rejected without falling back to its instance. Processor control
transactions do not create history points. Direct state writes to
instance-owned processor bypass or solo values are rejected.

The processor panel also exposes an `R` control. A regular click sends a local
reset for the complete processor. Ctrl/Command-click sends the same reset with
group scope, using the focused bank to resolve processor peers. Reset changes
are history-backed and restore each addressed value to its own initial value.
