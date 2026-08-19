# InteropSandbox Rules

Работа внутри `InteropSandbox` должна следовать этим правилам.

## Перед работой

1. Прочитать этот файл.
2. Прочитать [`Docs/README.md`](Docs/README.md) и найти нужный scope.
3. Всегда прочитать [`Docs/Rules.md`](Docs/Rules.md).
4. Всегда прочитать [`Docs/Formatting.md`](Docs/Formatting.md) и соответствующий языковой документ: [`Docs/CppFormatting.md`](Docs/CppFormatting.md), [`Docs/CSharpFormatting.md`](Docs/CSharpFormatting.md) или [`Docs/JavaScriptFormatting.md`](Docs/JavaScriptFormatting.md).
5. Если поведение или кодовый путь не понятен, сначала прочитать соответствующую документацию, затем исследовать код.

## Изменения

- Не добавлять fallback-пути, compatibility aliases, migration adapters, legacy branches, deprecated wrappers или duplicate APIs.
- Если встречен legacy-код, который больше не нужен текущей архитектуре, удалять его сразу, даже если он не относится напрямую к текущей задаче.
- Любая правка, добавленная как проверка гипотезы, должна быть удалена, если проверка показала, что она не исправляет проблему или основана на неверном предположении. Не оставлять такие правки как fallback, дополнительную защиту или вариант «на всякий случай»: кодовая база не должна накапливать поведение, для которого нет подтверждённой причины.
- Подтверждённую ошибку исправлять и сохранять независимо от того, была ли она найдена в ходе проверки гипотезы. Если причина не подтверждена, сохранять только минимальное изменение, которое действительно требуется наблюдаемым контрактом или воспроизводимым поведением.
- При добавлении функциональности обновлять соответствующую документацию в том же изменении.
- Документация не должна становиться огромным файлом: каждый самостоятельный scope хранить отдельно.
- Новый файл документации создавать и сразу добавлять в [`Docs/README.md`](Docs/README.md).
- Перед коммитом убедиться, что документация обновлена и соответствует коду.
- Если пользователь явно просит создать коммит, включать в него все текущие изменения рабочего дерева, включая незатреканные файлы и артефакты, без самостоятельного отбора.

## Запрещённые действия без разрешения

- Не собирать проект.
- Не запускать тесты или другие проверки.
- Не создавать коммиты.
- Не выполнять любые команды или операции Git, включая просмотр статуса и diff.

Эти действия разрешены только после явного запроса или разрешения пользователя.

## Сборка для агента

Все команды ниже выполнять только после явного разрешения пользователя на
сборку или публикацию. Команды запускаются из корня репозитория в Developer
PowerShell for Visual Studio. Для MSBuild использовать:

```text
C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe
```

### Отдельная сборка Native

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" Consolidator.Native\Consolidator.Native.vcxproj /t:Build /p:Configuration=Release /p:Platform=x64
```

Для отладочной сборки заменить `Release` на `Debug`.
Native target `CopyBinaryToExternals` после успешной сборки копирует
`ConsolidatorExternal.mxe64` в `Consolidator.Max\externals`.

### Отдельная публикация Managed

```powershell
dotnet publish Consolidator.Managed\Consolidator.Managed.csproj /p:PublishProfile=FolderProfile
```

Эта команда использует существующий Managed profile и не собирает Native.

### Сборка Native и публикация Managed

```powershell
& "C:\Program Files\Microsoft Visual Studio\18\Community\MSBuild\Current\Bin\MSBuild.exe" Consolidator.Managed\Consolidator.Managed.csproj /t:Publish /p:PublishProfile=NativeAndManaged /p:Configuration=Release /p:Platform=x64 /p:RuntimeIdentifier=win-x64 /p:SelfContained=true /p:PublishAot=true /p:NativeLib=Shared /p:PublishSingleFile=false /p:BuildNativeBeforePublish=true
```

Профиль `NativeAndManaged` нужно запускать через Visual Studio `MSBuild.exe`,
потому что обычный `dotnet publish` не предоставляет C++ targets. Профиль
сначала собирает Native в `Release|x64`, затем публикует Managed напрямую в
`Consolidator.Max\externals`. Профиль `FolderProfile` публикует только Managed
в отдельную папку `publish` и Native не собирает.

## Архитектура

Архитектурные границы, ABI, lifecycle, realtime и ownership описаны в
[`Docs/Rules.md`](Docs/Rules.md). Текущий Managed/Native/Max communication contract
описан в [`Docs/ManagedNativeCommunication.md`](Docs/ManagedNativeCommunication.md).
Не дублировать эти правила в `AGENTS.md`; при расхождении исправлять
архитектурную документацию и код.