# C++ Formatting

Правила форматирования и именования C++ кода в `InteropSandbox`.

## Naming

| Сущность | Стиль | Пример |
| --- | --- | --- |
| Class / struct | `PascalCase` | `ManagedBridge` |
| Enum / enum value | `PascalCase` | `NativeAtomType::Integer` |
| Method / function | `PascalCase` | `RegisterInstance()` |
| Local variable / parameter | `camelCase` | `frameCount` |
| Private field | `camelCase_` | `instanceId_` |
| Constant | `kPascalCase` | `kManagedLibraryName` |
| Namespace | lowercase | `consolidator::max` |
| Macro | `UPPER_SNAKE_CASE` | `MIN_EXTERNAL` |
| File | `PascalCase` | `ManagedBridge.cpp` |
| Type alias | `PascalCase` | `ManagedOutputCallback` |
| Template parameter | `PascalCase` | `ValueType` |

Не использовать Hungarian notation и бессмысленные сокращения. Общепринятые сокращения допустимы: `Dsp`, `Fft`, `Id`, `Abi`, `Api`, `Ui`.

## Files and includes

Одна основная сущность обычно получает пару `.h` / `.cpp`. Header содержит контракт, `.cpp` содержит реализацию. Большая реализация остаётся в header только для templates, `constexpr` и небольших inline-функций.

Использовать `#pragma once`.

Порядок include:

1. собственный header;
2. standard library;
3. external libraries;
4. project headers.

```cpp
#include "ManagedBridge.h"

#include <cstddef>
#include <cstdint>
#include <vector>

#include "ManagedInterop.h"
```

Каждый файл непосредственно подключает headers для используемых типов. Не использовать глобальный `using namespace std;`.

## Layout

Использовать Allman braces и `{}` для всех control flow конструкций.

```cpp
void ManagedBridge::SendMessage(
    InstanceId instanceId,
    const char* selector,
    const NativeAtom* atoms,
    std::size_t atomCount) const
{
}
```

Длинные initializer lists и вызовы переносить по логическим блокам. Guard clauses предпочтительнее глубокой вложенности.

В `switch` каждый `case` оформлять отдельным блоком. Если `case` объявляет локальные переменные, использовать дополнительные braces.

## Types and ownership

- Использовать `const` для неизменяемых значений и методов.
- Использовать `auto`, когда тип очевиден или чрезмерно длинный; явный тип оставлять, если он несёт смысл.
- Для owned resources использовать RAII и smart pointers.
- Raw pointers допустимы для non-owning references, nullable values, audio buffers, C ABI и interop memory.
- Не использовать raw pointer как неявный механизм владения.

## Attributes and exceptions

Использовать `[[nodiscard]]`, если игнорирование результата почти наверняка является ошибкой.

Использовать `noexcept`, когда функция действительно не должна бросать, особенно для C ABI callback handlers. Исключения не должны пересекать ABI.

Закрывать namespace явно:

```cpp
} // namespace consolidator::max
```

## ABI and runtime boundaries

ABI, callback, lifetime и realtime contracts являются архитектурными правилами,
а не правилами форматирования. Их источник находится в [`Rules.md`](Rules.md),
а текущая реализация обмена описана в
[`ManagedNativeCommunication.md`](ManagedNativeCommunication.md).
