# Processor Activity

The processor set now includes the Polish device in addition to input,
saturator, compressor, equalizer, and output. Polish activity is derived from
its `thick` and `air` macro values together with its bypass state.

Managed owns one local activity observer for the six instance-owned
processors and all seven equalizer banks. JavaScript presents the published
activity values directly. The row-level activity markers beside the track name
remain part of the presentation.

Each processor status contains `processorId`, `effectActive`, and `bypassed`.
Equalizer bank activity remains a
separate bank-owned status, calculated by the same observer.

Activity uses a small epsilon of `0.0001` to distinguish neutral from changed
control values. Input is active for non-zero `level`, Width different from its
neutral value `100`, or enabled `leveler`; saturator for
non-zero `drive` or `output`; compressor for non-zero `attack`, `sustain`,
`compression`, or `output`; Polish for non-zero `thick` or `air`; and output
for non-zero `level` or enabled `limiter`. The equalizer definition is
unchanged: a bank is active when it is not bypassed and contains at least one
non-bypassed filter whose gain exceeds that epsilon.

Processor changes are published as one revisioned
`registry_processor_changed` delta. A registry snapshot contains all six
statuses for every instance. The Max registry client applies deltas only when
the previous revision matches; a gap requests a complete snapshot.

Bank and processor activity are recalculated from state changes. The observer
publishes only transitions, while registry snapshots use the same activity
definition.

The panel navigation renders each processor using its own `effectActive` value:
inactive processors use disabled colors, and the processor color appears only
when the processor is active. Its separate navigation marker is not drawn.
The processor controls `B`, `R`, `BB`, and `BR` are disabled while the processor
is inactive. `BB` and `BR` remain available only for the equalizer panel.

Processor bypass uses a source-relative command:

```text
set_processor_bypass processorId local|group 0|1
```

Managed resolves the selected target and bank from the source instance's
`SelectionContext`; rendered row, instance, and bank IDs are not command
addresses. Group targets are resolved by `InstanceControlTargetResolver`. An
ungrouped bank is rejected without falling back to its instance. Processor control
transactions do not create history points. Direct state writes to
instance-owned processor bypass values are rejected. Device solo is not part of
the processor contract.

The processor panel also exposes an `R` control. A regular click sends a local
reset for the complete processor. Ctrl/Command-click sends the same reset with
group scope, using the focused bank to resolve processor peers. Reset changes
are history-backed and restore each addressed value to its own initial value.
When the equalizer panel is selected, its two panel controls are `BB` and `BR`:
`BB` bypasses the focused equalizer bank and `BR` resets that bank. They use the
same local/group scope and history behavior as the processor controls.
