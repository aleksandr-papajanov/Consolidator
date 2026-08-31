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
| [`UiPerformance.md`](UiPerformance.md) | Batching target snapshots и правила снижения Max JavaScript UI scheduler work. |
| [`ManagedProtocol.md`](ManagedProtocol.md) | Граница Managed protocol: atom decoding, command types, relative paths, routing scopes и response encoding. |
| [`BankIndexContract.md`](BankIndexContract.md) | Единый zero-based контракт индексов банков `0..6`. |
| [`Coordinator.md`](Coordinator.md) | Ownership общих Managed services, per-instance state и границы control/audio paths. |
| [`DspStatePipeline.md`](DspStatePipeline.md) | DSP snapshot publication, local JavaScript curves, analyzer configuration и FFT capture. |
| [`FilterCatalog.md`](FilterCatalog.md) | Filter kinds, parameter capabilities and legacy-derived defaults. |
| [`StateInfrastructure.md`](StateInfrastructure.md) | Независимые state tree, registry, history и observer contracts. |
| [`StateHistory.md`](StateHistory.md) | Managed state values, observers, topology peers, history и UI notifications. |
| [`Testing.md`](Testing.md) | Философия тестов, уровни, сквозные use cases, структура suites и команды запуска. |
| [`V8Migration.md`](V8Migration.md) | Инвентаризация Max JavaScript runtime и план перехода с `js`/`jsui` на `v8`/`v8ui`. |
| [`MaxV8RuntimeDiagnostics.md`](MaxV8RuntimeDiagnostics.md) | Диагностика загрузки Max for Live UI: LiveAPI lifecycle, activity, protocol snapshots, host-only проверки и пересборка `.amxd`. |
| [`ProcessorActivity.md`](ProcessorActivity.md) | Derived Managed processor activity status, registry deltas and processor B/S commands. |

## Правила работы с документацией

- Перед задачей прочитать этот индекс и документы, относящиеся к затрагиваемому scope.
- Если способ работы системы или кодовый путь не понятен, сначала искать объяснение в документации, затем читать реализацию.
- При добавлении функциональности обновлять соответствующую документацию в том же изменении.
- Не раздувать существующие документы: новый самостоятельный scope оформлять отдельным небольшим файлом.
- Новый файл документации сразу добавлять в эту таблицу.
- Перед коммитом проверить, что документация соответствует изменённому коду.

