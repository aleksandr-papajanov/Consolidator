# Managed State History

`StateHistory` is a singleton in the Managed DI container. It owns one logical
cursor for all active `ConsolidatorInstance` objects. Each `StateValue<T>` owns
`StateHistory.Capacity` values and uses the shared cursor as its index.

`AdvanceHistoryPoint()` has an explicit pre-operation contract: it opens a new
history point and must be called before the first write belonging to a logical
operation. It copies the current slot to the next slot for every registered
value and clears the redo range. Writing `StateValue<T>.Value` then changes only
the newly opened slot. Repeated writes to that slot are coalesced. Calling
`AdvanceHistoryPoint()` after the writes would copy the already modified state
and therefore cannot make those writes undoable.

`Undo()` and `Redo()` move the cursor without copying values and apply each
value's binding.

Bindings run while the history lock is held. They are deliberately
restricted to fast local Managed assignments or small deterministic projection
updates. A binding must not publish a DSP snapshot, send Max output, call the
Coordinator, start another history operation, perform I/O, or execute
long-running work. Coordinator-level work happens after the cursor operation
returns; for undo and redo, DSP publication is performed once per active
instance after all projections have completed.

`InstanceStateStore` owns the values for one instance and disposes them during
`ConsolidatorInstance.Stop()`. The store does not own history data.
A `StateValue<T>` uses an `IStateBinding<T>` to apply a logical value to its
projection. The first history-enabled value is the DSP gain. Its binding updates
the existing Managed `InstanceState`; future bindings may update derived state,
graph components, or non-DSP application state.

`InstanceStateBuilder` owns per-instance value registration and binding setup.
`ConsolidatorInstance` does not contain one setter method per parameter. DSP
compilation and native publication remain instance responsibilities.

History operations are control-path operations protected by a Managed lock.
Audio processing never accesses `StateHistory`. After a Coordinator undo or
redo, each active instance publishes one compiled DSP snapshot after all value
projections have been applied.