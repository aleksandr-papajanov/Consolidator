# InteropSandbox — правила разработки

## 1. Назначение проекта

`Consolidator.Native` — тонкий native-слой между Max и `Consolidator.Managed`.

Native отвечает только за:

- создание Max external;
- приём Max messages и atoms;
- преобразование Max atoms в простой interop-формат;
- вызов exported API `Consolidator.Managed`;
- приём callbacks из Managed;
- безопасную передачу Managed output в Max thread;
- приём аудио от Max;
- native realtime DSP;
- передачу аудиоданных в Managed для анализа;
- чтение опубликованного Managed DSP state.

В Native не размещаются:

- application state;
- routing;
- banks и groups;
- command handling;
- persistence;
- analysis logic;
- optimization;
- registry/coordinator logic.

Эти ответственности принадлежат `Consolidator.Managed`.

---

## 2. Статус проекта

Проект находится в активной разработке.

Обратная совместимость, legacy API, compatibility aliases, fallback-пути и параллельные реализации запрещены, если они явно не запрошены.

При изменении архитектуры:

1. вызывающие стороны переводятся на новую модель;
2. старый API удаляется;
3. неиспользуемый код удаляется сразу.

Запрещено:

- переносить старую архитектуру без необходимости;
- сохранять старые компоненты только ради совместимости.

---

## 3. Архитектурная граница

Основная зависимость:

```text
Max
 ↓
ConsolidatorExternal
 ↓
AtomCodec / ManagedBridge
 ↓
C ABI
 ↓
Consolidator.Managed
```

Realtime path:

```text
Max audio callback
 ↓
Native DSP
 ↓
audio output
```

Analysis input:

```text
Max audio callback
 ↓
короткий realtime-safe вызов Managed
 ↓
preallocated audio buffer / ring buffer
 ↓
Managed worker
 ↓
analysis
```

Managed output:

```text
Managed worker
 ↓
native callback
 ↓
native-owned output queue
 ↓
qelem
 ↓
Max outlet
```

Native не должен знать смысл Managed commands, state paths, banks, parameters или protocol semantics.

---

## 4. Interop API

Текущие имена exported entry points, callback signatures и конкретный поток
обмена описаны в [`ManagedNativeCommunication.md`](ManagedNativeCommunication.md).
Этот документ задаёт архитектурные инварианты; communication-документ не должен
дублировать общие правила ответственности и realtime.

Граница C++ / C# должна быть максимально простой.

Разрешены:

- fixed-width integers;
- `float` / `double`;
- pointers;
- `size_t` / `nuint`;
- POD-like structs;
- arrays через `pointer + count`;
- opaque IDs;
- unmanaged callbacks.

Не передавать через ABI:

- STL containers;
- C++ classes;
- Max/min-api types;
- managed objects;
- exceptions;
- `std::string`;
- C# `string`;
- сложные domain objects.

Calling convention должна быть явно одинаковой с обеих сторон.

Для Windows использовать `cdecl`, если API не определяет иное.

---

## 5. Atoms

Max atoms преобразуются в простой interop-формат.

Native поддерживает только необходимые типы:

```cpp
enum class NativeAtomType : std::uint8_t
{
    Integer = 1,
    Float = 2,
    Symbol = 3
};
```

Interop atom содержит:

- type;
- integer value;
- floating-point value;
- UTF-8 symbol pointer.

Native отвечает только за:

```text
Max atom → NativeAtom
```

Вся дальнейшая декодировка и интерпретация выполняется в Managed.

Запрещено реализовывать domain/protocol dispatch в Native.

---

## 6. Lifetime interop-данных

Pointers, полученные через callback или вызов ABI, считаются временными, если контракт явно не говорит обратного.

Если Managed вызывает Native callback:

```text
Managed memory
 ↓
callback
 ↓
Native немедленно копирует данные
 ↓
callback returns
```

Native не хранит переданные Managed string/array pointers после возврата callback.

Для долговременного обмена используется отдельно выделенная shared unmanaged memory.

---

## 7. Instance lifetime

Каждый Max external регистрируется в Managed при создании:

```text
RegisterInstance(...)
→ InstanceId
```

`InstanceId` используется только как идентификатор источника/назначения между Native и Managed.

При уничтожении external вызывается:

```text
UnregisterInstance(instanceId)
```

Контракт `UnregisterInstance`:

> После возврата функции Managed больше не может вызвать callback, связанный с уничтоженным external.

Native не должен получать callback через dangling `context` pointer.

---

## 8. Managed callbacks

Callback из Managed никогда напрямую не вызывает Max outlet.

Callback выполняет только:

1. проверку входных данных;
2. копирование данных в native-owned representation;
3. помещение frame в native queue;
4. `qelem.set()`.

Пример потока:

```text
Managed callback
 ↓
OutputFrame copy
 ↓
pendingOutput_
 ↓
qelem
 ↓
DrainManagedOutput()
 ↓
Max outlet
```

Native C++ callback handlers, которые вызываются из Managed через C ABI, должны
быть `noexcept` и поглощать исключения внутри native boundary.

C++ exceptions не должны пересекать ABI. Managed exported entrypoints также не
должны выпускать исключения в native caller; ошибки должны обрабатываться до
возврата из exported function.

---

## 9. Max thread boundary

Max API вызывается только из подходящего Max thread context.

Для Managed → Max используется один deferred queue/qelem на external.

Не создавать отдельный `defer()` для каждого Managed сообщения без необходимости.

`qelem` используется как сигнал:

```text
queue contains work
```

а сами сообщения хранятся в native-owned queue.

---

## 10. Audio thread

Audio callback является realtime-critical.

В audio callback запрещены:

- allocations;
- `new`;
- mutex;
- blocking synchronization;
- waits;
- file/network I/O;
- logging;
- string operations;
- Max message output;
- FFT;
- optimization;
- general Managed application logic.

Разрешены:

- native DSP;
- чтение заранее опубликованного DSP snapshot;
- atomic operations;
- копирование в заранее выделенный audio buffer;
- короткий realtime-safe interop call.

---

## 11. Managed audio input

Если аудио передаётся Managed для анализа, exported Managed audio entrypoint должен быть realtime-safe.

Он может:

- скопировать samples в заранее выделенный buffer;
- записать данные в bounded ring buffer;
- обновить atomic metadata.

Он не должен:

- выполнять FFT;
- выполнять analysis;
- аллоцировать память;
- использовать DI-driven application logic;
- логировать;
- блокироваться.

Analysis выполняется позже на Managed worker thread.

---

## 12. DSP state

Managed application state не читается напрямую из audio thread.

Managed публикует подготовленный DSP state через realtime-safe boundary.

Предпочтительная модель:

```text
Managed state
 ↓
compile DSP snapshot
 ↓
double buffer / atomic publish
 ↓
Native audio thread
 ↓
copy latest snapshot
 ↓
DSP
```

Через realtime boundary проходят только простые runtime-данные, необходимые DSP.

Native DSP не должен знать о:

- banks;
- groups;
- routing;
- UI state;
- command model;
- persistence.

---

## 13. Logging

Native и Managed logging не должны смешиваться с realtime processing.

Managed logging использует локальный logging abstraction проекта. Он может
передавать сообщения в Max через native log callback, но не должен вызываться
из audio callback или Managed realtime audio entrypoint.

Для вывода Managed logs в Max допускается native callback:

```text
Managed logging abstraction
 ↓
native log callback
 ↓
Max console
```

Логирование запрещено из audio callback и Managed realtime audio entrypoint.

---

## 14. Структура проекта

```text
InteropSandbox/
├─ Consolidator.Native/
│  └─ External/
│     ├─ ConsolidatorExternal.*
│     ├─ ManagedBridge.*
│     ├─ ManagedInterop.h
│     └─ AtomCodec.*
│
├─ Consolidator.Managed/
│  ├─ Core/
│  │  ├─ Abstractions/
│  │  └─ Instances/
│  ├─ Native/
│  └─ Protocol/
│
└─ Docs/
```

`Consolidator.Native/External/` содержит Max/min-api integration и native
interop bridge для текущего sandbox.

`Consolidator.Managed/Core/` содержит application orchestration.
`Consolidator.Managed/Core/Abstractions/` содержит managed application contracts.
`Consolidator.Managed/Core/Instances/` содержит per-external lifecycle state.
`Consolidator.Managed/Native/` содержит C# ABI exports, NativeAOT boundary и
marshaling из native representation в managed protocol types.

Будущие DSP-компоненты не должны зависеть от Max и Managed application logic.

---

## 15. Зависимости

Разрешено:

```text
ConsolidatorExternal → ManagedBridge / AtomCodec
ConsolidatorExternal → Min API
ManagedBridge → platform/native API
```

Запрещено:

```text
DSP → Max
DSP → ManagedBridge
Native external → Max protocol/domain logic
```

Max/min-api headers не должны попадать в DSP.

---

## 16. Размер и ответственность

Один класс — одна основная ответственность.

Примеры:

- `ConsolidatorExternal` — Max object lifecycle и wiring;
- `ManagedBridge` — загрузка Managed library и вызов exported API;
- `AtomCodec` — Max atoms ↔ interop atoms;
- DSP processor — только соответствующий DSP algorithm.

Не создавать god objects.

Не переносить application orchestration в `ConsolidatorExternal`.

---

## 17. Заголовки

`.h` описывает контракт.

Реализация размещается в `.cpp`, кроме коротких очевидных inline методов.

Минимизировать includes.

Не включать Max/min-api headers туда, где они не нужны.

---

## 18. Современный C++

Использовать C++20.

Предпочитать:

- RAII;
- `std::unique_ptr`;
- `std::array`;
- `std::span`;
- `std::vector` для owned dynamic storage;
- `std::atomic`;
- fixed-width integer types.

Raw pointers допустимы для:

- non-owning references;
- interop;
- audio buffers;
- callback context.

Raw pointers не используются для неявного владения.

---

## 19. Именование

| Элемент | Стиль | Пример |
|---|---|---|
| Классы / структуры | PascalCase | `ManagedBridge`, `OutputFrame` |
| Методы | PascalCase | `SendMessage()`, `RegisterInstance()` |
| Переменные | camelCase | `frameCount`, `instanceId` |
| Поля | camelCase_ | `managed_`, `instanceId_` |
| Параметры | camelCase | `atomCount`, `outputCallback` |
| Константы | `kPascalCase` | `kManagedLibraryName` |
| Макросы | UPPER_SNAKE_CASE | `MIN_EXTERNAL` |
| Файлы | PascalCase | `ManagedBridge.cpp` |

Имена должны отражать реальную ответственность.

---

## 20. Форматирование

Правила форматирования вынесены в отдельный документ: [`Formatting.md`](Formatting.md).

---

## 20.1. Visual Studio publish workflow

В `Consolidator.Managed/Properties/PublishProfiles/` доступны два профиля:

- `FolderProfile` — публикует только Managed NativeAOT library;
- `NativeAndManaged` — запускается через Visual Studio `MSBuild.exe`, сначала собирает `Consolidator.Native.vcxproj` в конфигурации `Release|x64`, затем публикует Managed и копирует `ConsolidatorExternal.mxe64` и `Consolidator.Managed.dll` в `Consolidator.Max/externals`. Обычный `dotnet publish` не может выполнить этот профиль, потому что не предоставляет C++ targets.

---

## 21. Комментарии

Комментарии объясняют:

- ответственность;
- lifetime;
- ownership;
- threading;
- ABI invariants;
- причины нетривиальных решений.

Не комментировать очевидный код.

Особенно явно документировать:

- кто владеет pointer;
- сколько pointer действителен;
- с какого thread вызывается callback;
- можно ли callback вызывать после unregister;
- realtime-safe ли функция.

---

## 22. Ошибки

Interop ошибки должны обнаруживаться как можно раньше.

Проверять:

- успешность загрузки Managed DLL;
- наличие всех exports;
- null callback;
- null pointers;
- неизвестные atom types;
- invalid InstanceId.

Ошибки ABI не должны скрываться.

Exceptions не пересекают C ABI.

---

## 23. Тесты

Тестировать прежде всего границы.

Тестовые проекты разделены по уровням:

- `Tests/Managed/Consolidator.Managed.Tests.csproj` — component tests для C# protocol, atom decoding, state и managed lifecycle;
- `Tests/Native/Consolidator.Native.Tests.vcxproj` — component tests для C++ ABI structs, atom conversion и native ownership/lifetime helpers;
- `Tests/Integration/Consolidator.Integration.Tests.csproj` — integration tests для опубликованной Managed NativeAOT library, exports и Managed/Native contract.

Не переносить component assertions в integration project. Integration tests
могут требовать предварительной публикации Managed и должны проверять реальную
границу, а не private implementation details.

Минимальные integration tests:

- Managed DLL загружается;
- все exports находятся;
- instance register/unregister работает;
- Max atoms корректно преобразуются в `NativeAtom`;
- Managed callback корректно копируется в native output frame;
- callback после unregister невозможен;
- shared DSP state читается корректно;
- audio transfer не выполняет allocations/locks в realtime path.

DSP algorithms тестируются отдельно от Max и Managed.

---

## Главное правило

`Consolidator.Native` должен оставаться маленьким.

Если новая логика не требует:

- Max API;
- native DSP;
- realtime boundary;
- C++/C# interop,

то почти наверняка ей место в `Consolidator.Managed`, а не в Native.