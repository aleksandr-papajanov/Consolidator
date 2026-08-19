# Documentation Index

Документация `InteropSandbox` разделена по scope. Каждый отдельный архитектурный или workflow-вопрос должен иметь свой небольшой файл, а не добавляться в один большой документ.

| Файл | Содержание |
| --- | --- |
| [`Rules.md`](Rules.md) | Архитектурные границы, ответственность C++/C#/JavaScript, ABI, lifecycle, realtime, зависимости, ошибки и тестовые требования. |
| [`Formatting.md`](Formatting.md) | Общие правила форматирования, выбор языка/слоя и tooling. |
| [`CppFormatting.md`](CppFormatting.md) | Форматирование, naming, includes, ownership и ABI-boundary conventions для C++. |
| [`CSharpFormatting.md`](CSharpFormatting.md) | Форматирование, naming, using directives, managed dependencies и NativeAOT interop conventions для C#. |
| [`JavaScriptFormatting.md`](JavaScriptFormatting.md) | ES5-compatible Max runtime style, include order, callbacks, lifecycle, protocol и UI layer conventions для JavaScript. |
| [`ManagedNativeCommunication.md`](ManagedNativeCommunication.md) | Текущий контракт связи Managed/C++/Max: ABI atoms, callbacks, queue/qelem, ownership и unregister barrier. |
| [`Coordinator.md`](Coordinator.md) | Lifetime и ownership общего Managed Coordinator, per-instance state и границы control/audio paths. |

## Правила работы с документацией

- Перед задачей прочитать этот индекс и документы, относящиеся к затрагиваемому scope.
- Если способ работы системы или кодовый путь не понятен, сначала искать объяснение в документации, затем читать реализацию.
- При добавлении функциональности обновлять соответствующую документацию в том же изменении.
- Не раздувать существующие документы: новый самостоятельный scope оформлять отдельным небольшим файлом.
- Новый файл документации сразу добавлять в эту таблицу.
- Перед коммитом проверить, что документация соответствует изменённому коду.
