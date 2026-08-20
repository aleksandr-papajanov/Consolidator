# Managed State History

`StateHistory` is a singleton in the Managed DI container. It owns one logical
cursor for all active `ConsolidatorInstance` objects. Each `StateValue<T>` owns
`StateHistory.Capacity` values and uses the shared cursor as its index. This
includes each instance's `InstanceState`: seven bank slots indexed from 0 to 6,
where every slot may contain a nullable `GroupId`. `InstanceState` also stores
an optional focused `BankAddress`. The focused bank may belong to another
instance, so it is a reference rather than an index constrained to the owning
instance.

State code is organized by responsibility: shared history primitives and
`InstanceStateStore` live in `Core.State`, with generic bindings in
`Core.State.Bindings`. Topology is a separate feature under `Core.Topology`.

The managed state address model lives in `Core.State.StatePath`. It exposes
typed factories for instance fields, bank groups, DSP parameters, and DSP
markers, plus immutable `WithNode`, `WithParameter`, and `WithMarker` helpers.
`StatePath.Matches` retains the legacy prefix-query behavior without exposing
fixed native arrays or optional C++ fields to application code. `StateEntry`
contains the addressed value and write metadata; `StateMetadata` describes
which fields are persistent and undoable.

The full DSP tree is represented by the single `DspState` root and the small
`ParameterState`/`StateMarker` primitives kept beside it in `Core.Dsp`.
`TopologyState`
continues to own grouped banks and focused-bank navigation, while
`InstanceState` owns instance label, selected bank, audibility, and the
reference to the focused bank. Native interop must use a separate ABI DTO and
must not depend on these domain types.

`TopologyStore` maintains derived group indexes from the history-backed
topology values. The indexes map each `GroupId` to its bank addresses and to
the participating instance IDs. They are updated by the topology binding, so
undo and redo restore both the topology values and the search indexes.

`AdvanceHistoryPoint()` has an explicit pre-operation contract: it opens a new
history point and must be called before the first write belonging to a logical
operation. It copies the current slot to the next slot for every registered
value and clears the redo range. Writing `StateValue<T>.Value` then changes only
the newly opened slot. Repeated writes to that slot are coalesced. Calling
`AdvanceHistoryPoint()` after the writes would copy the already modified state
and therefore cannot make those writes undoable.

`Undo()` and `Redo()` move the cursor without copying values and apply each
value's binding.

Bindings run after the history lock is released. They are deliberately
restricted to fast local Managed assignments or small deterministic projection
updates. A binding must not start another history operation, perform I/O, or
execute long-running work. Coordinator-level operation serialization keeps
another logical control operation from interleaving during this projection
window. For undo and redo, DSP publication is performed once per active
instance after all projections have completed.

`InstanceStateStore` owns the values for one instance and disposes them during
`ConsolidatorInstance.Stop()`. The store does not own history data.
A `StateValue<T>` uses an `IStateBinding<T>` to apply a logical value to its
projection. The gain binding updates the existing Managed `InstanceState`,
while the topology binding updates the Coordinator-owned topology projection.
Other bindings may update derived state, graph components, or non-DSP
application state.

`TopologyIndex` is an internal implementation detail of `TopologyStore` for
derived group indexes.
It maps each `GroupId` to its bank addresses and to the participating instance
IDs. `TopologyBinding` updates it after the history lock is released, so undo
and redo restore topology values and then update the search indexes.

`Coordinator` owns the current topology projection and the topology history
values for registered instances. Topology updates are control-path operations;
they do not enter the audio path or DSP snapshot.

`InstanceStateBuilder` owns per-instance value registration and binding setup.
`ConsolidatorInstance` does not contain one setter method per parameter. DSP
compilation and native publication remain instance responsibilities.

History operations are control-path operations serialized by the Coordinator's
operation lock. `StateHistory` still protects its own cursor and ring buffers,
but that internal lock does not define the atomicity of a complete logical
operation. Audio processing never accesses `StateHistory`. After a Coordinator
undo or redo, each active instance publishes one compiled DSP snapshot after
all value projections have been applied.