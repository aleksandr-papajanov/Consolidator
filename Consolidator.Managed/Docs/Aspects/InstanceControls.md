# Instance controls and registry status

Instance controls use the same target resolution for mute and solo. A grouped
bank resolves to all members, while an ungrouped bank resolves to its single
instance. In the bank manager, `S` is exclusive on a regular click and becomes
additive with Shift-click; `M` and `B` are always additive, so regular clicks
can accumulate mute or bypass across instances. If the clicked instance's
focused bank belongs to a group, additive operations keep group scope and add
or remove the complete group. A non-additive solo gesture keeps the current
scope; an ungrouped instance is treated as a single-instance group. Selecting
another grouped bank is the explicit way to solo a different group.

Registry reads current managed state
and returns a structured atom sequence. Each registry instance entry carries
its instance-level `mute` and `solo` values. Each registry bank entry also carries
`effectActive`, which is true when the bank is not bypassed and has an active
filter with non-zero gain. Each bank entry also carries its raw `bypassed`
value. Managed emits `registry_bank_effect_changed` and
`registry_bank_bypass_changed` when the corresponding bank status changes;
instance mute and solo changes use their corresponding registry deltas. Instance controls use dedicated
`set_instance_mute`, `set_instance_solo`, and `set_instance_bypass` commands;
direct protocol writes to the instance `mute`, `solo`, and `bypass` paths are
rejected. Each command carries an
explicit target instance ID, `local` or `group` scope, and `exclusive` or
`additive` selection mode. Bank bypass controls carry an explicit bank index.
Managed resolves that target through `ContextualBankResolver`. Group resolution never
traverses another group on the same track and falls back to the target instance
when the bank is ungrouped. Both handlers update the resolved
local instance values in one atomic state transaction without creating a
history point. Later topology changes do not migrate the stored mute or solo
values. Command handlers remain in Core and do not move routing or state
mutation into codecs.

After the common command-frame header, the request bodies are:

```text
set_instance_mute instanceId local|group 0|1 exclusive|additive
set_instance_solo instanceId local|group 0|1 exclusive|additive
set_instance_bypass instanceId local|group 0|1 exclusive|additive
```

Processor/bank statuses and bypass commands: [ProcessorActivity](ProcessorActivity.md).
Target observation: [TargetObservation](TargetObservation.md).
