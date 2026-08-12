# Consolidator Max JS

## Назначение

`Consolidator.Max/` — новый рабочий домен Max-интерфейса. Папка `Max/` остаётся
legacy-источником контекста и не подключается как runtime-зависимость.

Актуальная структура JS:

```text
js/
├── Clients/
│   ├── ConsolidatorClient.js
│   ├── StateClient.js
│   ├── AnalysisClient.js
│   └── NativeProtocolClient.js
└── ViewModels/
    ├── ConsolidatorViewModel.js
    ├── StateValueViewModel.js
    ├── AnalyzerViewModel.js
    ├── EqualizerViewModel.js
    ├── BankViewModel.js
    ├── FilterViewModel.js
    ├── CompressorViewModel.js
    ├── SaturatorViewModel.js
    ├── GainViewModel.js
    └── ObservableValue.js
```

## Публичная граница

`ConsolidatorClient` предоставляет две независимые части:

- `client.state` — чтение, запись, reset и подписки на authoritative state;
- `client.analysis` — выбор global `AnalysisView`, единый `tick()` и подписки на
  spectrum, EQ curves и telemetry.

Основной API:

```js
var threshold = client.state.get("compressor.threshold");
client.state.fetch("compressor.threshold", updateThreshold);
client.state.set("compressor.threshold", -18);
client.state.set("equalizer.bank.2.filter.4.gain", 3);
client.state.reset("compressor");
client.state.subscribe("compressor.threshold", updateThreshold);

client.state.getFor(instanceId, "equalizer.bank.2.filter.4.gain");

client.analysis.view(instanceId, bankId);
client.analysis.tick();
client.analysis.subscribe("spectrum.main", drawSpectrum);
client.analysis.subscribe("eq.filter.4", drawFilter);
client.analysis.subscribe("meter.compressor", drawCompressorLevel);
client.analysis.subscribe("compressor.reduction", drawReduction);
client.analysis.subscribe("saturator.distortion", drawDistortion);
```

`fetch()` передаёт callback entry собственного instance и вторым аргументом
ошибку response: `(entry, errorResponse)`. При успехе второй аргумент равен
`null`. Callback `set()` может получить полный response с `entries` и
`byInstance`, что важно для grouped writes.

Внутренняя граница состоит из четырёх слоёв:

```text
ConsolidatorClient
├── StateClient
│   ├── cache[instance][path]
│   ├── requests
│   └── subscriptions
├── AnalysisClient
│   ├── currentView
│   ├── cache[key]
│   └── subscriptions
└── NativeProtocolClient
    ├── Max framing
    ├── request IDs
    ├── pending
    └── selector dispatch/listeners
```

`NativeProtocolClient.on()` допускает несколько listeners на один selector и
возвращает функцию снятия конкретного listener-а.

## ViewModel layer

`Consolidator.Max/js/ViewModels` содержит UI projections поверх client layer;
transport и client code находится в `Consolidator.Max/js/Clients`.

- `StateValueViewModel` — writable state value с value/ranges/status и
  lifecycle unsubscribe;
- `ObservableValue` — read-only analysis value;
- `GainViewModel`, `SaturatorViewModel`, `CompressorViewModel` и
  `EqualizerViewModel` — projections видимых controls. EQ создаёт только один
  `currentBank` с семью фильтрами и перепривязывает его при `showBank(bankId)`;
- `AnalyzerViewModel` — spectrum, curves и telemetry;
- `ConsolidatorViewModel` — root aggregation и `selectBank(bankId)` для
  собственного source instance.

Контроллы работают с ViewModel-объектами и не знают canonical state paths,
analysis keys или native protocol. `StateValueViewModel.set()` делегирует write
в `StateClient`, а `AnalyzerViewModel` передаёт только готовые analysis values.
Каждый ViewModel имеет `destroy()` для снятия подписок. State value получает
`loaded === true` только после первого native entry; immediate subscription не
вызывает callback до этого момента.

`selectBank(bankId)` и `analyzer.show(instanceId, bankId)` намеренно разделены:
первый меняет authoritative `selected_bank` собственного instance, второй лишь
меняет global analysis view и не переводит parameter controls на чужой instance.

State projections покрывают текущие native paths для compressor (`gain`, `mix`,
`bypass`, `solo`), saturator (`gain`, `detectorAmount`, `bypass`, `solo`),
filters (`bypass`, `solo`) и selected EQ bank (`bypass`, `solo`, `group`).

`ConsolidatorViewModel.initialize(callback)` выполняет initial sync: device/global
state и `selected_bank` читаются batches не более 16 entries, после чего
загружается только выбранный bank. `selectBank(bankId)` меняет selected state и
лениво загружает новый bank; остальные шесть banks не создаются и не читаются.

State использует canonical dot-paths (`compressor.threshold`,
`equalizer.bank.2.filter.4.gain`). Cache индексируется по `instanceId`; обычные
операции используют default instance, а `getFor()` и `subscribeFor()` позволяют
явно выбрать instance.

Значение `selected_bank` нормализуется в JS как публичное число `1..7`, даже
если native response кодирует его как `bank1..bank7`.

Ответ state-запроса хранит `entries` массивом и дополнительно строит индекс
`byInstance[instanceId][path]`. Поэтому grouped response с одинаковым path для
нескольких instances не теряет entries. Callback `fetch()` получает entry
собственного source instance.

Analysis не смешивается с state. `analysis.view(instanceId, bankId)` меняет
отображаемый глобальный analyzer и не изменяет `state.selected_bank`.

Wire selectors analysis явно сопоставляются с JS-ключами. Например,
`eq_all_banks` публикуется как `eq.allBanks`; автоматическое преобразование
имён transport-слоя запрещено.

`AnalysisClient` хранит `currentView`. Повторный вызов `view()` с теми же
`instanceId` и `bankId` не отправляет native-команду и не инвалидирует cache.
При смене view подписчики получают `null` для всех ранее опубликованных ключей,
чтобы UI успел очистить старые графики до прихода новых frames.
Каждое новое analysis value также содержит snapshot `view` с `instanceId` и
`bankId`. Native frames несут `viewRevision`, `instanceId`, `bankId`, затем
payload. JS
отбрасывает frame, если instance или bank не совпадают с `currentView`, а
также frame с revision старше уже наблюдаемой эпохи этого view. При переходе
на более новую эпоху cache инвалидируется перед публикацией нового значения.
Wire-level `viewRevision` и `instanceId` кодируются decimal symbols тем же
безопасным ID-кодеком, что и control protocol.

Все `subscribe()`-методы возвращают функцию снятия подписки. Для state это
также относится к `subscribeFor()`, поэтому UI может закрывать lifecycle без
повторного хранения path и callback. Третий аргумент `immediate === true`
сразу передаёт callback уже закэшированное значение, если оно существует.

## Native transport

Control frames идут через `read`, `write`, `reset` и принимаются через
`client.handleControl()`. Analysis frames идут через `analysis_view`,
`analysis_tick` и принимаются через `client.handleAnalysis()`. Эти две boundary
точки соответствуют native `controlOutput` и `analysisOutput`; protocol-клиент
скрывает version, source и request id от UI-кода.

JS использует `PROTOCOL_VERSION = 1`. Native decoder и response encoder также
используют version `1` для всех control response frames.

## Сборки

При включённой CMake-опции
`CONSOLIDATOR_COPY_BUILT_EXTERNALS_TO_MAX=ON` native external и рядом лежащая
`ConsolidatorCore` копируются в `Consolidator.Max/externals/`. Вручную запускать
сборку не требуется: штатная точка входа — `.vscode/build-all.cmd`.

Client contract-test находится в `Consolidator.Max/tests/ClientTests.js` и
запускается через Node.js. Production-файлы дополнительно проходят
`node --check`; сами они сохраняют глобальный ES5-style API для Max JS.
