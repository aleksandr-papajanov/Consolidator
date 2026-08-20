# Managed Coordinator

## Lifetime

`Coordinator` регистрируется в Managed DI container как singleton:

```text
ServiceProvider
    -> one Coordinator
        -> many ConsolidatorInstance
```

Его lifetime равен lifetime DI container и загруженной
`Consolidator.Managed.dll`. Все Max externals внутри одного процесса и одной
загруженной Managed DLL получают доступ к одному `Coordinator`.

`ConsolidatorCore` использует `Coordinator`, но не владеет его lifecycle.
Остановка одного external не останавливает общий Coordinator и не затрагивает
другие instances.

## Ownership

`Coordinator` владеет общим Managed coordination state:

- instance IDs;
- registration и unregistration;
- control-message routing;
- per-instance prepare routing;
- cross-instance operations.

`ConsolidatorInstance` владеет только состоянием конкретного external:

- lifecycle state;
- `DspState` and its `InstanceStateStore`;
- per-instance state registration through `InstanceStateBuilder`;
- `IInstanceOutput` transport;
- `IDspStatePublisher` for the native-owned exchange;
- instance-specific services и context.

`StateHistory` is a Managed DI singleton shared by the Coordinator and all
active instances. An instance must receive that history explicitly; production
constructors do not create a private history. `InstanceStateBuilder` creates
the instance's `StateValue<T>` registrations and their `IStateBinding<T>`
projections. A binding may update a simple field, derived state, a graph
component, or non-DSP application state; bindings run after the history lock is
released.

Topology values are registered in the shared history through `TopologyStore`,
not in `InstanceStateStore`. `TopologyStore` is a Managed DI singleton and
owns the global topology feature, including per-instance topology history
values, topology mutations, and topology queries. `InstanceRegistry` is a separate
Managed DI singleton that owns `InstanceId -> ConsolidatorInstance` references;
it has no topology or history responsibilities. `TopologyIndex` is a separate
Managed DI singleton for derived group indexes and graph queries. `Coordinator`
coordinates registration, unregister lifecycle, history navigation, and public
operations across these services.

`DspStateCompiler` is a stateless Managed DI singleton. It is shared by the
Coordinator and instances and only derives runtime snapshots from an
instance's current Managed state; it does not own per-instance state.

`ConsolidatorInstance`, `Coordinator` и `ConsolidatorCore` depend only on the
managed `IDspStatePublisher` abstraction. The pointer-based
`NativeDspStatePublisher` implementation remains in the Native boundary and
is created by `NativeApi`.

`DspState` is authoritative Managed DSP state. `DspStateCompiler` derives the
fixed-layout `DspSnapshot` from it, and only that runtime snapshot is published
to Native. `Prepare(sampleRate, maximumFrameCount)` is the lifecycle-safe place
to update the DSP compilation context; it must not reset parameter state. The
current gain-only prototype has no sample-rate-dependent values yet, while
future attack/release coefficients and filter coefficients belong in this
prepare-time compilation.

После `UnregisterInstance` соответствующий instance ID больше не используется.
Следующая регистрация получает новый ID.

`UnregisterInstance` сначала удаляет instance из общего registry, затем вызывает
`ConsolidatorInstance.Stop()`. Lifecycle gate внутри instance синхронизирует
`Stop()` и `TrySend()`: если output callback уже выполняется, unregister ждёт его
завершения. После возврата `UnregisterInstance` callback для этого instance больше
не может быть вызван.

## Threading

`Coordinator` serializes one logical control operation at a time through a
private operation lock. This lock covers:

- `RegisterInstance`;
- `UnregisterInstance`;
- `ReceiveMessage`;
- `Prepare`;
- topology mutations;
- `AdvanceHistoryPoint`;
- `UndoHistory` и `RedoHistory`.

History bindings run after the internal `StateHistory` lock is released. The
Coordinator operation lock keeps the history cursor, stored values, bindings,
topology indexes, and DSP publication in one logical order, so another control
operation cannot interleave between the history write and its projection.
Coordinator does not hold this lock during the audio callback. Output callback
execution is protected only by the per-instance lifecycle gate.

`ReceiveAudio` не обращается к Coordinator и не входит под этот lock. Audio
ingestion должен использовать per-instance realtime-safe reference или slot и не
ждать control path.

## Boundary

Singleton действует только внутри одного процесса и одной загруженной Managed DLL.
Для общего state между процессами потребовался бы отдельный IPC или persistence
механизм; `Coordinator` этого не предоставляет.