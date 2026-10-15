# C# Formatting

Правила форматирования и именования C# кода в `Consolidator.Managed`.

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

## Type and member conventions

Поля, не меняющиеся после construction, объявлять readonly; публичные данные —
properties. Collection contract выбирать по использованию: ReadOnlySpan для
синхронного заимствования, IReadOnlyList для indexed access, IEnumerable для
действительной lazy enumeration. Не оставлять unused using.

Architecture и dependency policy находятся в [Rules](Rules.md) и
[Architecture](Architecture.md); ABI annotations/layout — в
[InteropAbi](../../Docs/Aspects/InteropAbi.md).