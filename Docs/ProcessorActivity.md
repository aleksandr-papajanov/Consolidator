# Processor Activity

Managed owns the derived activity state for the five instance-owned
processors: `input`, `saturator`, `compressor`, `equalizer`, and `output`.
JavaScript only presents the published values.

Each processor status contains `processorId`, `effectActive`, `bypassed`, and
`soloed`. Input and output do not support solo; their `soloed` value is always
false and their solo control is disabled. Equalizer bank activity remains a
separate bank-owned status.

Processor changes are published as one revisioned
`registry_processor_changed` delta. A registry snapshot contains all five
statuses for every instance. The Max registry client applies deltas only when
the previous revision matches; a gap requests a complete snapshot.

Processor bypass and solo use the explicit commands:

```text
set_processor_bypass processorId targetInstanceId instance 0|1
set_processor_bypass processorId targetInstanceId group bankId 0|1
set_processor_solo processorId targetInstanceId instance 0|1 exclusive|additive
set_processor_solo processorId targetInstanceId group bankId 0|1 exclusive|additive
```

Group targets are resolved by `InstanceControlTargetResolver`. An ungrouped
bank is rejected without falling back to its instance. Processor control
transactions do not create history points. Direct state writes to
instance-owned processor bypass or solo values are rejected.
