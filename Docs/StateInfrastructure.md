# State Infrastructure

`Consolidator.Managed/State` is the reusable state mechanism. It is outside
`Core` and must not depend on application concepts such as instances, banks,
groups, topology, DSP, routing or protocol delivery.

```text
State/
├─ History/     shared cursor, transactions and history slots
├─ Observers/   IStateValueObserver<TValue> contract
├─ Tree/        NodeId and heterogeneous state nodes
├─ StatePath.cs
├─ StateRegistry.cs
└─ StateValue.cs
```

`StateRegistry<TRootId>` accepts any non-null root identifier type. It owns
root trees, creates history-backed or transient leaves, registers values with
`StateHistory`, and disposes a root's values on removal. Callers supply an
ordered observer list; the registry does not discover or construct application
observers.

Root lookup is synchronized. Structural tree construction and removal belong
to the caller's control-operation boundary; the application serializes them
with `IOperationGate`.

`StateValue<TValue>` owns value storage, history integration and observer
lifecycle. It stores the requested value unchanged; bounds belong to concrete
application policy. Tree nodes provide structural lookup and typed leaf access.
Runtime-typed reads and writes belong directly to the Core command handlers
that require them. These types contain no DI or Core policy.

The application boundary is `Core/State/StateValueFactory`. It wraps
`StateRegistry<InstanceId>` and decides:

- local versus connected edit scope;
- copy versus delta editing and physical ranges;
- instance-owned versus bank-owned notifications;
- peer and state-change observers;
- additional observers requested by concrete state models.

Concrete models and observers remain in `Core/State`. Routing and Protocol may
consume infrastructure contracts such as `StatePath`, but business behavior
must not move back into the top-level mechanism.
