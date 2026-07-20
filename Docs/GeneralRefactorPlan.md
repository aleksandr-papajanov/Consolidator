# Consolidator: поэтапный план общего рефакторинга

## 1. Назначение

Этот план переводит целевую архитектуру из `GeneralRefactor.md` в семь крупных
этапов. Каждый этап является самостоятельной рабочей итерацией и рекомендуемой
границей commit. Внутри этапа задачи выполняются последовательно сверху вниз.

Целевой результат:

- `DeviceHost` является единственным владельцем официального состояния;
- компоненты отправляют Host типизированные команды и получают подтвержденные
  events/snapshots;
- runtime-протокол использует Max atoms, а не Dictionary envelopes;
- Dictionary используется только для persistence;
- DSP получает компактные immutable snapshots;
- Analyzer и Approximator не изменяют EQ и не знают друг о друге;
- высокочастотные analysis data идут по отдельному bounded stream;
- JS управляет UI, но не владеет domain state.

## 2. Обязательные решения

1. Целевая архитектура из `GeneralRefactor.md` имеет приоритет над разделами
   `AGENTS.md`, описывающими текущий envelope/StateStore flow. `AGENTS.md`
   обновляется одновременно с фактической сменой контракта.
2. Compatibility layer запрещен. Нельзя оставлять legacy readers, aliases,
   fallback formats или два варианта одной runtime-команды.
3. Новую инфраструктуру разрешено строить отключенной. Во время runtime
   cutover старый путь удаляется полностью в той же итерации.
4. Typed C++ `FilterOptions` является единственным источником filter
   definitions, ranges, mappings и defaults. JSON хранит только Max-specific
   presentation: цвета, scripting names, позиции и видимость.
5. Runtime-протокол не использует JSON, Max Dictionary, имена Dictionary,
   `MessageEnvelope`, `source/target/payload` или dynamic message objects.
6. Persistence migrations на стадии разработки не создаются. Несовместимая
   schema сбрасывается к typed defaults.
7. Bank IDs и filter IDs one-based во всех слоях. UI может менять порядок
   отображения, но не преобразует ID.
8. Domain state, snapshots, DSP и fit results содержат абсолютные значения.
   Нормализованные `0..1` существуют только между Max controls и UI controller.
9. Store revision увеличивается ровно один раз после успешного atomic commit.
   Protocol version, store revision и operation/session ID не смешиваются.
10. Audio externals строят локальную immutable processing model на message
    thread и меняют ее realtime-safe snapshot swap.
11. Analyzer-to-Approximator live data не проходит через DeviceHost. Для него
    используется bounded latest-value/SPSC transport с одним producer и одним
    consumer.
12. Не создавать abstraction ради названия. Store, workflow или отдельный
    проект появляется только при наличии самостоятельной ответственности.

## 3. Правила выполнения

- Перед этапом прочитать `AGENTS.md`, `GeneralRefactor.md`, этот план и все
  затрагиваемые исходники целиком.
- Проверить `git status`; пользовательские изменения сохранить и не откатывать.
- Выполнять только выбранный этап, не начинать следующий.
- Перед изменением public contract найти все usages через `rg`.
- Не менять Max patch wiring, если текущий пункт этого прямо не требует.
- Не редактировать `.mxe64` вручную и не коммитить binary отдельно от source.
- Все command inlets/outlets и JS ports документировать полным списком команд.
- PascalCase использовать для типов и методов, camelCase для переменных и
  полей.
- После этапа старшая модель выполняет полный review/build/tests и создает
  commit. Младшая модель во время реализации не отвлекается на полные сборки.

## 4. Карта замены текущего кода

| Сейчас | Проблема | После рефакторинга |
| --- | --- | --- |
| `consolidator.eqstorage.js` | одновременно state, definitions, normalization, persistence и routing | `DeviceHost` + `EqStore` + `PersistenceService`; JS только UI |
| `DeviceStateStore.js` | общий mutable Dictionary и generation | typed stores и scoped revisions |
| `BusHub` | routing и startup barrier | тонкий atom transport к/от Host |
| `MessageEnvelope`/`MessageFactory` | Dictionary runtime protocol | typed Atom Codec |
| `ComponentHost` | target routing и чтение общего Dictionary | typed atom endpoint/snapshot receiver |
| `DeviceStateDictionaryCodec` | Dictionary как runtime state API | persistence codec only |
| `EqChain` | читает полный DeviceState | получает только `DspSnapshot` |
| `Analyzer` | читает полный DeviceState | получает `AnalyzerSnapshot` |
| `Approximator` | читает DeviceState и отправляет filter mutations | получает `FitInputSnapshot`, возвращает `FitResult` Host |
| Filter endpoint JS | пересылает envelopes | controller отправляет typed Host commands |

# Этап 1. Контракты, тестовая основа и Domain

Цель этапа: зафиксировать новый протокол и создать transport-neutral domain
foundation, не меняя работающий Max runtime.

## 1.1. Baseline

- [ ] Записать текущий `git status --short` и не включать посторонние изменения
  в commit этапа.
- [ ] Зафиксировать в `Docs/GeneralRefactorBaseline.md` текущие рабочие сценарии
  и известные ошибки: device load, filter edit, bank CRUD/select, Listen, Fit,
  Live Set save/reopen.
- [ ] Составить список всех существующих runtime message selectors и их
  producer/consumer.
- [ ] Составить список всех мест чтения/записи Max Dictionary вне persistence.

## 1.2. Atom protocol specification

- [ ] Создать `Docs/AtomProtocol.md`.
- [ ] Зафиксировать command header:
  `command <version> <source> <requestId> <commandName> <typed fields...>`.
- [ ] Зафиксировать event header:
  `event <version> host <eventId> <eventName> <typed fields...>`.
- [ ] Зафиксировать snapshot header:
  `snapshot <version> host <storeName> <revision> <typed fields...>`.
- [ ] Удалить из целевого формата свободный `target`: commands всегда идут
  Host, events/snapshots всегда публикует Host.
- [ ] Для variable arrays определить явное count framing.
- [ ] Описать diagnostics: unknown selector, wrong version/arity/type, invalid
  enum, invalid ID и invalid transition.
- [ ] Описать MVP commands: component attach/detach, EQ set/reset, bank
  add/remove/rename/select, analyzer listen, fit start/cancel/clear,
  persistence restore/commit.
- [ ] Описать store/operation events и definitions/EQ/DSP/analyzer/fit
  snapshots.
- [ ] Для каждого сообщения привести валидный и невалидный пример.

## 1.3. Native tests

- [ ] Создать `Consolidator.Tests` как обычный CTest executable, не Max
  external.
- [ ] Использовать простой runner или уже доступный Catch; новую package
  dependency не добавлять.
- [ ] Добавить helpers для сравнения states, curves и atom sequences.
- [ ] Зафиксировать regression tests текущей `ParameterRange`, filter defaults,
  bank/filter order и magnitude response gain/peak/shelves/tilt.

## 1.4. `Consolidator.Domain`

- [ ] Создать target без Max SDK, DSP implementation и NLopt.
- [ ] Создать каталоги `Ids`, `Enums`, `Definitions`, `States`, `Snapshots`,
  `Commands`, `Events`, `Operations`, `Results`.
- [ ] Добавить однозначные types для `BankId`, `FilterId`, `ComponentId`,
  `RequestId`, `EventId`, `SessionId`, `StoreRevision`, `ProtocolVersion`.
- [ ] Перенести `FilterType`, `ParameterRange`, `FilterDefinition`,
  `FilterState`, `EqBank`, `EqState` и `EqSnapshot` из Shared/Models.
- [ ] Snapshot должен содержать собственную revision; старый transport
  `generation` в Domain не переносить.
- [ ] Добавить `FindBank`, `FindFilter`, const overloads и typed default factory
  из `FilterOptions`.
- [ ] Добавить минимальные `AnalyzerState`, `ApproximatorState`, operation status
  enums, `FitCandidate` и `FitResult`.
- [ ] Добавить специализированные immutable `DspSnapshot`, `AnalyzerSnapshot`
  и `FitInputSnapshot`.
- [ ] Добавить typed command/event structs для всех сообщений из
  `AtomProtocol.md`.
- [ ] Не помещать Max atoms, Dictionary, JS data или DSP formulas в Domain.
- [ ] Покрыть IDs, defaults, lookup, equality и command construction tests.

## 1.5. Документация этапа

- [ ] Обновить `AGENTS.md` только в части уже созданных Domain contracts и
  tests. Не описывать новый runtime как уже подключенный.
- [ ] Записать mapping старых моделей на новые, чтобы следующий этап не создавал
  дубликаты.

Критерий готовности: новый Domain полностью независим от Max; protocol
зафиксирован до реализации codec; существующий runtime не изменен.

# Этап 2. DSP Core, Atom Messaging и transactional stores

Цель этапа: закончить переиспользуемый native foundation, на котором Host будет
строить state и общаться с Max.

## 2.1. `Consolidator.DspCore`

- [ ] Создать target и перенести из Shared DSP/Audio/Curve/Eq/Spectrum код.
- [ ] Сохранить раздельные left/right instances для stateful stereo devices.
- [ ] Обновить Analyzer, Approximator и EqChain includes без forwarding headers
  и alias targets.
- [ ] Оставить Shared только для действительно общей инфраструктуры либо
  удалить его после последнего пользователя.
- [ ] Сохранить behavior через regression tests этапа 1.

## 2.2. Snapshot builders

- [ ] Добавить `DspSnapshotBuilder`: все банки в ascending ID order, bypass и
  только необходимые DSP fields.
- [ ] Добавить `AnalyzerSnapshotBuilder`: selected bank, selected prefix и
  total EQ response inputs.
- [ ] Добавить `FitInputBuilder`: definitions, captured bank и selected-bank
  baseline на момент запуска fit.
- [ ] Builders не читают Max Dictionary, JSON или runtime transport.
- [ ] Покрыть порядок банков, bypass, selected-bank/prefix/total projections.

## 2.3. `Consolidator.Messaging`

- [ ] Создать transport-neutral `AtomValue` variant и `AtomList`.
- [ ] Реализовать `AtomReader`: cursor, typed reads, enum/count parsing и
  `RequireEnd`.
- [ ] Реализовать симметричный `AtomWriter`.
- [ ] Ошибка decode содержит code, field index и expected/actual type.
- [ ] Создать отдельные codecs для command/event/snapshot families.
- [ ] Создать `CommandCodec`/`EventCodec` registry без giant string switch в
  Host.
- [ ] Проверять category, protocol version, arity, enum и array counts.
- [ ] В MaxAdapter добавить только преобразование
  `c74::min::atoms <-> AtomList`; domain codecs не должны знать Max.
- [ ] Добавить positive round-trip и negative protocol tests для каждого
  message type.

## 2.4. Store infrastructure

- [ ] Создать `Consolidator.DeviceHost` target без Max SDK.
- [ ] Ввести `UpdateResult` со статусами Changed/Unchanged/Rejected и typed
  error.
- [ ] Ввести immutable snapshot publisher/observer interface.
- [ ] Revision увеличивать только внутри успешного commit и только при
  фактическом изменении.

## 2.5. `EqStore`

- [ ] Инициализировать state из typed definitions/defaults.
- [ ] Реализовать set parameter, reset filter, bank add/remove/rename/select.
- [ ] Реализовать batch apply для fit result одной транзакцией.
- [ ] Валидировать IDs и clamp values через definitions.
- [ ] Сохранять one-based IDs и ascending storage order.
- [ ] Зафиксировать policy bank ID после удаления; предпочтительно не
  переиспользовать ID.
- [ ] Покрыть success/no-op/rejected и single-revision batch tests.

## 2.6. Operation stores

- [ ] Реализовать `AnalyzerStore`: session status, progress, aggregate/result,
  error.
- [ ] Реализовать `ApproximatorStore`: status, candidate, loss, result, error.
- [ ] Не хранить FFT frames, mouse state и optimizer iterations.
- [ ] Создать `GlobalStore` только при наличии конкретных устойчивых полей.

## 2.7. Документация этапа

- [ ] Обновить CMake ownership и `AGENTS.md` по фактически перенесенным
  Domain/DspCore/Messaging/Store contracts.
- [ ] Не менять Max runtime protocol на этом этапе.

Критерий готовности: Domain, DspCore, Messaging и stores тестируются без Max;
существующие externals используют новую DSP foundation, но старый runtime еще
работает без второго активного протокола.

# Этап 3. DeviceHost, persistence и operation workflows

Цель этапа: реализовать полный in-memory центр состояния и его persistence до
подключения к рабочему Max flow.

## 3.1. DeviceHost facade

- [ ] Реализовать `DeviceHost` как фасад над stores, handlers, publisher,
  registry, workflows и persistence port.
- [ ] Добавить отдельные `EqCommandHandler`, `AnalyzerCommandHandler`,
  `ApproximatorCommandHandler`, `ComponentCommandHandler`.
- [ ] `DeviceHost::Handle` принимает typed command, не atoms.
- [ ] Routing выполняется через typed registry/visitor, без giant string switch.
- [ ] Handler выполняет validate -> prepare -> atomic commit -> publish.
- [ ] Компоненты не получают mutable references на stores.

## 3.2. Component registry и lifecycle

- [ ] Реализовать attach/detach с ID, type, protocol version и capabilities.
- [ ] Required components задать typed options Host, а не BusHub JS.
- [ ] Late attach получает актуальные relevant snapshots.
- [ ] Повторный attach имеет deterministic contract и tests.
- [ ] Не переносить глобальный ready/start barrier в новый Host.

## 3.3. Notifications и coalescing

- [ ] Публиковать `store.updated` только после commit.
- [ ] Публиковать full snapshot при attach, restore и resync.
- [ ] Возвращать confirmed incremental EQ update инициатору.
- [ ] Для тяжелых consumers coalesce частые updates в latest snapshot, не
  теряя последнюю revision.
- [ ] Добавить integration tests с fake UI/DSP/Analyzer/Approximator.

## 3.4. Persistence service

- [ ] Создать `Consolidator.Persistence`.
- [ ] Определить `PersistedDeviceState`: schema version и устойчивые domain
  states без runtime revisions/sessions/progress/caches.
- [ ] Реализовать transport-neutral `PersistenceCodec`.
- [ ] Max Dictionary conversion оставить единственной Max-specific persistence
  boundary.
- [ ] При пустой/несовместимой schema создавать typed defaults без migration.
- [ ] Restore завершать до `HostInitializedEvent`.
- [ ] Commit выполнять после store commit с debounce.
- [ ] Покрыть round-trip: one/multiple banks, selected bank, bypass, names,
  corrupted input и wrong schema.

## 3.5. Operation state machine

- [ ] Ввести общий operation contract: session ID, state и допустимые
  transitions start/progress/complete/fail/cancel.
- [ ] Невалидный transition возвращает typed error без изменения store.
- [ ] Stale result с чужим session ID не изменяет state.

## 3.6. `FitWorkflow`

- [ ] На fit start фиксировать immutable `FitInputSnapshot` и session ID.
- [ ] Реализовать states Starting/Capturing/Approximating/Validating/Applying/
  Completed/Failed/Cancelled.
- [ ] Candidate/final result сначала коммитить в `ApproximatorStore`.
- [ ] Auto-apply применять к captured bank одной EqStore transaction.
- [ ] После apply публиковать Eq update, DSP snapshot и operation completion.
- [ ] Реализовать cancel/failure cleanup.
- [ ] Покрыть success, cancel, stale result, deleted bank и invalid result.
- [ ] Не создавать BankSwitchWorkflow, если смена банка остается одной
  транзакцией.

## 3.7. Документация этапа

- [ ] Обновить `AGENTS.md` по Host/stores/persistence/workflows как native
  foundation, еще не объявляя Max cutover завершенным.

Критерий готовности: in-memory Host проходит initialize -> restore -> attach ->
EQ edit -> fit -> persistence round-trip без Max и прямого общения components.

# Этап 4. Max DeviceHost и единый runtime cutover

Цель этапа: одним согласованным изменением переключить рабочее устройство с
Dictionary envelopes на Host/atoms. После этапа старого runtime-протокола нет.

## 4.1. DeviceHost Max feature

- [ ] Создать native project/output `consolidator.devicehost.mxe64`.
- [ ] Создать `Max/Features/DeviceHost/DeviceHost.maxpat` и один root
  `consolidator.devicehost.controller.js`.
- [ ] Первый command inlet принимает только atom commands из protocol spec.
- [ ] Первый outlet публикует atom events/snapshots; второй direct status/error.
- [ ] Persistence Dictionary имеет отдельные private inlet/outlet и никогда не
  попадает на runtime bus.
- [ ] Добавить полный inlet/outlet assist contract.

## 4.2. Transport hub и root wiring

- [ ] Свести BusHub к transport: component commands -> Host, Host outputs ->
  scoped `---` bus.
- [ ] Удалить startup barrier, participant state и domain routing из BusHub.
- [ ] Добавить DeviceHost feature в `Max/Consolidator.amxd`.
- [ ] Переподключить root `pattr`/`pattrstorage` только к Host persistence.
- [ ] Проверить isolation нескольких device instances через scoped names.

## 4.3. Filter UI

- [ ] Удалить `consolidator.filter.js`, если после перехода он остается только
  forwarding endpoint.
- [ ] Controller преобразует normalized UI gestures в typed
  `eq.set_parameter`/`eq.reset_filter` commands.
- [ ] Definitions/ranges/defaults получать от Host; JSON читать только для
  layout/colors/scripting names.
- [ ] Controls обновлять только confirmed Host update/snapshot.
- [ ] Реализовать feedback suppression и сохранить bypass/reset behavior.

## 4.4. Bank UI

- [ ] Перевести controller/list на typed add/remove/rename/select commands.
- [ ] Строить список из `EqSnapshot`, не хранить domain-копию банков в JS.
- [ ] Сохранить reverse visual order, но click отправляет реальный one-based ID.
- [ ] Удалить bank state, definitions, normalization и persistence из
  `consolidator.eqstorage.js`.
- [ ] Переименовать feature, если имя EqStorage больше не соответствует
  ответственности.

## 4.5. Native endpoints

- [ ] Заменить старый `ComponentHost` на atom endpoint/snapshot receiver.
- [ ] EqChain временно получает только `DspSnapshot` atoms.
- [ ] Analyzer получает `AnalyzerSnapshot` и operation commands.
- [ ] Approximator получает `FitInputSnapshot`/operation commands.
- [ ] Ни один consumer не открывает runtime Dictionary.

## 4.6. Удалить старый runtime protocol

- [ ] Удалить JS `MessageEnvelope`, `MessageFactory` и includes.
- [ ] Удалить native envelope/factory messages и
  `MessageEnvelopeDictionaryCodec`.
- [ ] Удалить `DeviceStateChangedMessage` и runtime
  `DeviceStateDictionaryCodec`.
- [ ] Удалить selectors `message <dictionary>` и старые patch cords.
- [ ] Удалить `source`, `target`, `broadcast`, `stateName`, `generation`,
  `system.status`, `system.start` из runtime.
- [ ] Удалить old filter interfeature selectors; одно намерение имеет один
  typed Host command.
- [ ] Полностью переписать runtime sections `AGENTS.md` под фактический flow.

## 4.7. Проверка этапа старшей моделью

- [ ] Clean build и CTest.
- [ ] Проверить patcher JSON, dependencies, assists и отсутствие out-of-range
  patchcords.
- [ ] Smoke: new/restored instance, controls, bank CRUD/select, persistence и
  два независимых instances.
- [ ] `rg` не находит runtime usages `MessageEnvelope`,
  `device.state.changed`, `system.start` и component Dictionary readers.

Критерий готовности: рабочий Max device использует только Host/atoms; старый
runtime path удален, persistence сохранено через единственную Host boundary.

# Этап 5. Unified DSP Engine и Analyzer

Цель этапа: завершить realtime processing boundary и перевести Analyzer на
специализированный snapshot вместо глобального state.

## 5.1. Unified DSP external

- [ ] Переименовать EqChain в `Consolidator.DspExternal`, если topology уже
  шире EQ; alias external не оставлять.
- [ ] DSP external принимает stereo audio и только `DspSnapshot` updates.
- [ ] Chain строится на message thread вне audio callback.
- [ ] Left/right используют независимые stateful device instances.
- [ ] Реализовать immutable processing model и realtime-safe atomic/bounded
  snapshot swap.
- [ ] Не выполнять allocation, Dictionary access, serialization или blocking
  lock в audio processing.
- [ ] Добавить parameter smoothing только там, где он нужен DSP behavior.
- [ ] Покрыть bank order, bypass, channel independence и snapshot replacement.

## 5.2. Analyzer domain boundary

- [ ] Разделить feature extraction, operation state и visual snapshot builder.
- [ ] Analyzer принимает main/reference audio и Host-issued session commands.
- [ ] EQ responses получает только в `AnalyzerSnapshot` от Host.
- [ ] Удалить чтение DeviceState/Dictionary и самостоятельный выбор банков.
- [ ] Selected bank, selected prefix и total response semantics формируются
  одним shared builder и покрываются tests.

## 5.3. Analyzer outputs и SpectrumView

- [ ] Progress/completed result отправлять Host typed events.
- [ ] Не отправлять каждый FFT frame Host.
- [ ] SpectrumView получает prepared visual snapshot через direct presentation
  port/controller.
- [ ] Listen off сразу очищает difference smoothing, retained difference и
  visual layer.
- [ ] Painter не читает Dictionary, definitions или official state.
- [ ] Сохранить marker selection/cycling/draw order behavior.

## 5.4. Verification

- [ ] Проверить FFT frequency alignment и общую frequency grid.
- [ ] Проверить current/reference/difference semantics.
- [ ] Проверить selected prefix/current, selected-bank baseline и total EQ.
- [ ] Проверить signal passthrough, bank order, rapid drag, sample-rate change и
  DSP restart.
- [ ] Обновить `AGENTS.md` и Max assists.

Критерий готовности: DSP и Analyzer получают только свои typed snapshots; audio
thread realtime-safe; Analyzer не знает Approximator, EqStore или UI controls.

# Этап 6. Approximator stream, полный fit flow и UI synchronization

Цель этапа: завершить live analysis/fit workflow без прямых component commands
и убрать остатки domain logic из UI.

## 6.1. Specialized analysis stream

- [ ] Определить typed `AnalysisChunk`.
- [ ] Реализовать bounded latest-value или SPSC transport с одним Analyzer
  producer и одним Approximator consumer.
- [ ] Capacity и overflow policy хранить в typed options.
- [ ] Считать dropped chunks; не сохранять chunks в stores.
- [ ] Host создает/закрывает session и endpoints, но не проксирует chunks.
- [ ] Очистить stream при cancel/failure/new session.

## 6.2. Approximator execution

- [ ] Получать immutable `FitInputSnapshot` на start.
- [ ] Worker не читает mutable Host, Dictionary или UI state.
- [ ] Публиковать progress/candidate/result с session ID.
- [ ] Не отправлять filter updates и не менять DSP напрямую.
- [ ] Host валидирует result и применяет его через `FitWorkflow` к captured
  bank.
- [ ] UI selection во время fit не меняет target bank операции.

## 6.3. Fit UI

- [ ] Fit/Listen controls отражают operation snapshot, а не локально
  вычисленный ready state.
- [ ] Один click запускает одну session.
- [ ] During processing controls имеют однозначное состояние.
- [ ] Cancel/error/completion возвращают UI в состояние Host snapshot.
- [ ] Candidate визуализируется отдельно от official DSP state до apply.

## 6.4. Общий UI слой

- [ ] Выбрать один `DeviceUiController` либо несколько тонких feature
  controllers над общей UI utility; domain ownership запрещен в обоих случаях.
- [ ] Создать control registry по stable parameter IDs и scripting names.
- [ ] Definitions получать от Host, presentation metadata из JSON.
- [ ] Удалить duplicated normalization, validation, defaults и bank storage из
  JS.
- [ ] Для marker drag разрешить transient preview, но confirmed Host value
  всегда заменяет его без snap-back lag.
- [ ] Painters хранят только visual snapshot и transient interaction state.

## 6.5. Tests и performance

- [ ] Deterministic fit на fixed input/seed.
- [ ] Repeated fit без накопления ошибки.
- [ ] Cancel, stale result, invalid result и deleted captured bank.
- [ ] Max main thread не блокируется optimizer.
- [ ] Rapid marker/control drag не создает feedback loop и UI jumps.
- [ ] Обновить `AGENTS.md`, assists и operation docs.

Критерий готовности: ordinary fit lifecycle проходит только через Host
workflow; live data идет только через bounded stream; result применяется одной
EqStore transaction.

# Этап 7. Удаление legacy, полный аудит и финальная приемка

Цель этапа: удалить все остатки прежней архитектуры и доказать целостность
нового flow.

## 7.1. Удалить мертвый код

- [ ] Удалить старый EqStorage state JS, `DeviceStateStore.js`, `BankFilter.js`
  и Dictionary helpers без persistence-only usages.
- [ ] Удалить старые envelope contracts, serializers, codecs и message names.
- [ ] Удалить лишние BusHub/controller layers, если transport выражается
  обычными scoped `s/r` и Host endpoint.
- [ ] Удалить unused CMake targets, includes и copied runtime files.
- [ ] Удалить дубли definitions/ranges/defaults из JSON и JS.
- [ ] Не оставлять forwarding headers, aliases, migrations и fallback readers.

## 7.2. Архитектурный аудит

- [ ] `Dict`, `DictionaryReader`, `MaxDictionarySerializer` встречаются только
  в Persistence/Max persistence adapter.
- [ ] В runtime нет `source/target/broadcast/payload` envelope contract.
- [ ] Components не содержат имен и commands других components.
- [ ] Только Host изменяет official stores.
- [ ] Только typed C++ definitions содержат ranges/mappings/defaults.
- [ ] DSP callback не вызывает allocation, waitable mutex, serialization или
  Max object API.
- [ ] Session IDs, revisions и protocol version используются строго по
  назначению.
- [ ] Painters/UI не владеют domain state.
- [ ] Все command ports имеют полный assist contract.

## 7.3. Полная проверка

- [ ] Clean configure/build всех targets.
- [ ] Полный CTest.
- [ ] `git diff --check` и проверка compile warnings.
- [ ] Проверить Max patcher syntax, dependencies, missing objects и patchcord
  ranges.
- [ ] Smoke matrix: new/restored/two instances, bank CRUD/select/rename,
  parameter and marker drag, bypass/reset, Listen, Fit, cancel/error, Live Set
  save/reopen, sample-rate change и DSP restart.
- [ ] Проверить отсутствие Max console errors и duplicate events.
- [ ] Проверить CPU/UI responsiveness при drag, analysis и fit.
- [ ] Проверить persistence isolation и schema reset behavior.

## 7.4. Документация и commit

- [ ] Полностью сверить `AGENTS.md` с фактическим кодом и удалить описание
  старой архитектуры.
- [ ] Обновить `GeneralRefactor.md`, если реализация обоснованно отличается от
  исходного target design.
- [ ] Обновить protocol, persistence, Max assists и build documentation.
- [ ] Зафиксировать финальную dependency/project tree.
- [ ] Создать финальный cleanup commit только после успешной приемки.

Критерий готовности: в проекте один runtime protocol, один владелец official
state, один persistence boundary, один DSP snapshot path и один управляемый fit
workflow.

# Рекомендуемые commits

1. `refactor: establish domain and protocol contracts`
2. `refactor: add dsp messaging and store foundations`
3. `feat: add device host persistence and workflows`
4. `refactor: switch max runtime to device host`
5. `refactor: unify dsp and analyzer snapshots`
6. `refactor: complete hosted fit workflow`
7. `refactor: remove legacy architecture`

Существующее постороннее изменение `.gitignore` не включать в commit этапа,
если оно не относится к поставленной задаче.

# Definition of done этапа

Этап принимается старшей моделью только если:

- выполнены все его пункты без TODO и временных adapters;
- изменения не выходят в следующий этап;
- public contracts отражены в коде, assists и документации;
- добавлены требуемые tests;
- clean build и релевантные tests проходят;
- Max загружается без новых ошибок, если этап затрагивает Max;
- пользовательские посторонние изменения не затронуты;
- `git diff --check` проходит;
- `AGENTS.md` описывает фактически реализованное состояние;
- создан один изолированный commit этапа либо явно перечислены блокеры.
