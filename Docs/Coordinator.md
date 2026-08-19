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

`Coordinator` владеет общим Managed application state и registry:

- instance IDs;
- `ConsolidatorInstance` lookup;
- registration и unregistration;
- control-message routing;
- per-instance prepare routing;
- cross-instance operations.

`ConsolidatorInstance` владеет только состоянием конкретного external:

- lifecycle state;
- `IInstanceOutput` transport;
- instance-specific services и context.

После `UnregisterInstance` соответствующий instance ID больше не используется.
Следующая регистрация получает новый ID.

`UnregisterInstance` сначала удаляет instance из общего registry, затем вызывает
`ConsolidatorInstance.Stop()`. Lifecycle gate внутри instance синхронизирует
`Stop()` и `TrySend()`: если output callback уже выполняется, unregister ждёт его
завершения. После возврата `UnregisterInstance` callback для этого instance больше
не может быть вызван.

## Threading

Coordinator synchronizes короткие операции control path через private lock:

- `RegisterInstance`;
- `UnregisterInstance`;
- `ReceiveMessage`;
- `Prepare`.

Coordinator не держит общий lock во время output callback. Callback защищается
только per-instance lifecycle gate.

`ReceiveAudio` не обращается к Coordinator и не входит под этот lock. Audio
ingestion должен использовать per-instance realtime-safe reference или slot и не
ждать control path.

## Boundary

Singleton действует только внутри одного процесса и одной загруженной Managed DLL.
Для общего state между процессами потребовался бы отдельный IPC или persistence
механизм; `Coordinator` этого не предоставляет.