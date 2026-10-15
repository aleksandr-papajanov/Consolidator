# Interop и lifetime

Общие инварианты Managed/Native/host. Текущий ABI — в [InteropAbi](InteropAbi.md).
Host delivery — в [VST3 Host](../../Consolidator.Vst3/Docs/Aspects/Host.md).

## Interop API

Текущие имена exported entry points, callback signatures и конкретный поток
обмена описаны в [InteropAbi](InteropAbi.md).
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
- host/framework objects;
- managed objects;
- exceptions;
- `std::string`;
- C# `string`;
- сложные domain objects.

Calling convention должна быть явно одинаковой с обеих сторон.

Для Windows использовать `cdecl`, если API не определяет иное.


## Lifetime interop-данных

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


## Instance lifetime

Каждый host instance регистрируется в Managed при создании:

```text
RegisterInstance(...)
→ InstanceId
```

`InstanceId` используется только как идентификатор источника/назначения между Native и Managed.

При уничтожении host instance вызывается:

```text
UnregisterInstance(instanceId)
```

Контракт `UnregisterInstance`:

> После возврата функции Managed больше не может вызвать callback, связанный с уничтоженным host instance.

Native не должен получать callback через dangling `context` pointer.


## Managed callbacks

Callback немедленно копирует borrowed данные в native-owned representation.
Доставка выполняется позже на JUCE message thread. Из callback не вызывать UI напрямую.
Host-specific доставка описана в документации соответствующего проекта.

C++ callback handlers через C ABI должны быть `noexcept` и обрабатывать исключения
внутри boundary. Managed exported entrypoints тоже не выпускают exception в
native caller. Диагностика следует [общим правилам](../Rules.md).

Audio-thread constraints: [RealtimeBoundary](RealtimeBoundary.md).
