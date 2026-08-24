# C# Formatting

Правила форматирования и именования C# кода в `InteropSandbox`.

## Naming

| Сущность | Стиль | Пример |
| --- | --- | --- |
| Class / struct / record | `PascalCase` | `NativeOutput` |
| Interface | `IPascalCase` | `IProtocolTransport` |
| Enum / member | `PascalCase` | `AtomType.Symbol` |
| Method | `PascalCase` | `RegisterInstance()` |
| Property | `PascalCase` | `InstanceId` |
| Local variable / parameter | `camelCase` | `atomCount` |
| Private field | `_camelCase` | `_callback` |
| Constant | `PascalCase` | `MaximumFrameCount` |
| Namespace | `PascalCase` | `Consolidator.Managed.Native` |
| File | `PascalCase` | `NativeOutput.cs` |
| Generic parameter | `T` + name | `TValue` |

Acronyms рассматриваются как обычные слова: `DspState`, `FftAnalyzer`, `NativeApi`, `InstanceId`.

## Files and using directives

Предпочитать один основной тип на файл и file-scoped namespace.

```csharp
using System.Runtime.InteropServices;

using Consolidator.Managed.Protocol;

namespace Consolidator.Managed.Native;
```

`System.*` dependencies идут раньше project dependencies. Не оставлять unused `using`.

## Layout and methods

Использовать Allman braces и `{}` для всех control flow конструкций. Длинные сигнатуры переносить по одному параметру на строку:

```csharp
public static unsafe void SendAudio(
    ulong instanceId,
    double* mainLeft,
    double* mainRight,
    nuint frameCount)
{
}
```

Использовать guard clauses вместо ненужной вложенности:

```csharp
if (instance is null)
{
    return;
}

instance.Reset();
```

Использовать `var`, когда тип очевиден. Явный тип предпочтителен, когда он улучшает понимание.

## State and dependencies

- Предпочитать `readonly` для полей, которые не меняются после construction.
- Публичные данные обычно предоставлять через properties.
- Application dependencies передавать через constructor injection.
- Не использовать Service Locator внутри domain/application классов.
- `ILogger<T>` и обычные managed abstractions не смешивать с native pointers.

## Interop

`unsafe`, pointers, `delegate* unmanaged`, `Marshal`, `UnmanagedCallersOnly`, `StructLayout` и `FieldOffset` должны оставаться близко к `Consolidator.Managed/Native`.

NativeAOT entrypoints должны быть маленькими: проверить ABI input, преобразовать representation, вызвать managed component и вернуть управление. Business logic в entrypoint не размещать.

Calling convention должна совпадать с C++ стороной. Исключения не должны пересекать unmanaged boundary.

Для коллекционных параметров использовать самый узкий подходящий contract: `ReadOnlySpan<T>`, `IReadOnlyList<T>` или `IEnumerable<T>` в зависимости от семантики.

Для ABI structs layout важнее обычных object-oriented conventions. Правила
`NativeAtom` и декодирования описаны в [`Rules.md`](Rules.md); после декодирования
application code не должен зависеть от `NativeAtom`.

## Managed state and dependencies

В Managed application code разрешены обычные classes, records, dictionaries, lists, LINQ, DI, `ILogger<T>`, tasks, channels и async/await. Realtime restrictions относятся только к коду, который непосредственно вызывается из native audio callback.

Dependencies передавать через constructor injection:

```csharp
public sealed class ManagedService
{
    private readonly ILogger<ManagedService> _logger;
    private readonly StateStore _stateStore;

    public ManagedService(
        ILogger<ManagedService> logger,
        StateStore stateStore)
    {
        _logger = logger;
        _stateStore = stateStore;
    }
}
```

Не использовать Service Locator внутри domain/application классов и не передавать native pointers через application-level DI без необходимости.

## Managed abstractions

Application code не вызывает native callback напрямую. Использовать обычную
managed abstraction, например:

```csharp
namespace Consolidator.Managed.Protocol.Transport;

public interface IProtocolTransport
{
    void Send(ProtocolOutput message);
}
```

Конкретный ABI, NativeAtom layout, callback lifetime и realtime contract описаны
в [`Rules.md`](Rules.md) и [`ManagedNativeCommunication.md`](ManagedNativeCommunication.md).

## Collections and threading

Для параметров использовать самый узкий подходящий contract:

- `ReadOnlySpan<T>` — данные нужны только синхронно и не сохраняются;
- `IReadOnlyList<T>` — нужен обычный collection API;
- `IEnumerable<T>` — действительно нужна lazy enumeration semantics.

Threading ownership должен быть очевиден из архитектуры. Не добавлять mutex, queue, atomics или worker thread без конкретного concurrency requirement. Для каждой очереди явно определить producer, consumer, ownership, lifetime и overflow semantics.

Границы потоков и ownership очередей описаны в
[`ManagedNativeCommunication.md`](ManagedNativeCommunication.md).

## NativeAOT exceptions

Exception не должен пересекать unmanaged boundary. Если `[UnmanagedCallersOnly]` entrypoint вызывает код, который потенциально может бросить, exception обрабатывается внутри Managed boundary и при необходимости логируется. Не использовать exceptions как способ передачи ожидаемого protocol result обратно в C++.

## Tests

Тестировать прежде всего managed contracts и boundaries:

- `NativeAtom` decoding;
- command decoding и routing;
- state updates;
- instance registration и unregister lifecycle;
- output generation;
- analyzer и optimizer;
- malformed native input.

Application logic тестировать без NativeAOT boundary, используя обычные managed objects.

