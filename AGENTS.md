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
- Если результат тестов выявил проблему, исправлять саму проблему в production-коде или тестовой инфраструктуре после установления причины. Не изменять assertions и не ослаблять тест только для получения зелёного результата; сначала проверить, вызвана ли проблема текущими правками, нарушением контракта, гонкой, неверной изоляцией или ошибкой fixture.
- При добавлении функциональности обновлять соответствующую документацию в том же изменении.
- Документация не должна становиться огромным файлом: каждый самостоятельный scope хранить отдельно.
- Новый файл документации создавать и сразу добавлять в [`Docs/README.md`](Docs/README.md).
- Перед коммитом убедиться, что документация обновлена и соответствует коду.
- Если пользователь явно просит создать коммит, включать в него все текущие изменения рабочего дерева, включая незатреканные файлы и артефакты, без самостоятельного отбора.

## Запрещённые действия без разрешения

Для DSP действуют [Docs/Dsp/Rules.md](Docs/Dsp/Rules.md): запрещены создание и
запуск автоматических тестов аудиоалгоритмов, DSP-примитивов и runtime graph.
Ручные исследования/прослушивание через EmulationLab или Plugin Doctor — только
по явному запросу. Тесты ABI, protocol, persistence и транспорта имеют отдельный scope.

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

### VST3 Publish profile

The Managed project's Publish page has one profile: `Vst3Plugin`. Select it
and choose **Publish** in Visual Studio. It publishes the Managed NativeAOT
library and its dependencies directly to
`Consolidator.Vst3\build\managed-runtime`, then builds and installs the plugin
bundle and runtime DLLs to the configured system VST3 directory. It does not
build the transitional Max external.

Use Visual Studio MSBuild for Publish because the VST3 project uses Visual
Studio C++ targets. Set `SkipVst3Build=true` only when publishing Managed for
NativeAOT integration tests; it still publishes to the VST3 runtime directory,
which is also where the integration tests load the Managed DLL.

## Архитектура

Архитектурные границы, ABI, lifecycle, realtime и ownership описаны в
[`Docs/Rules.md`](Docs/Rules.md). Текущий Managed/Native/Max communication contract
описан в [`Docs/ManagedNativeCommunication.md`](Docs/ManagedNativeCommunication.md).
Не дублировать эти правила в `AGENTS.md`; при расхождении исправлять
архитектурную документацию и код.
