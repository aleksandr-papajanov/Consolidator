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
│   ├── RegistryClient.js
│   └── NativeProtocolClient.js
└── ViewModels/
    ├── ConsolidatorViewModel.js
    ├── StateValueViewModel.js
    ├── AnalyzerViewModel.js
    ├── EqualizerViewModel.js
    ├── BankViewModel.js
    ├── BankManagerViewModel.js
    ├── FilterViewModel.js
    ├── CompressorViewModel.js
    ├── SaturatorViewModel.js
    ├── GainViewModel.js
    └── ObservableValue.js
└── Presenters/
    ├── Core/PresentationObservable.js
    ├── Core/Normalization.js
    ├── Core/PresentationBinding.js
    ├── Dial/DialPresentation.js
    ├── Dial/DialPresenter.js
    ├── Button/ButtonPresentation.js
    ├── Button/ButtonPresenter.js
    ├── Slider/SliderPresentation.js
    ├── Slider/SliderPresenter.js
    ├── BankManager/BankManagerPresentation.js
    └── BankManager/BankManagerPresenter.js
├── Controllers/
    └── BankManagerController.js
└── Controls/
    ├── Dial/DialControl.js
    ├── Button/ButtonControl.js
    ├── Slider/SliderControl.js
    └── BankManager/BankManagerControl.js
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

## Presenter layer

`Presenters/` находится между ViewModels и будущими generic controls. Presenter
композирует state и analysis ViewModels в semantic presentation snapshot:
`DialPresenter` создаёт normalized rings, display metadata и telemetry, а
`ButtonPresenter` создаёт простой button snapshot. Presenters не используют Max
API, renderer или client напрямую. Их подписки снимаются через `destroy()`.

`BankManagerPresenter` принимает только semantic `BankManagerViewModel`: rows,
banks, link groups, focus, edit mode и action state. Scroll position не входит в
presentation и хранится только Control-ом как viewport interaction state. Он не знает legacy manager,
`visibilityPolicy`, `groupOperations` или Max APIs. `BankManagerControl` получает
готовые `rows`, `banks`, `linkGroups`, `editAction` и `clearAction` и выполняет
только layout, drawing и hit testing.

`BankManagerController(viewModel, rootViewModel)` принимает generic control intents и решает application
semantics: обычный bank selection, link-edit selection, group application,
clear confirmation. Scroll остаётся локальным viewport state Control и не
становится application intent. Clear confirmation живёт в Controller:
первый `clearRequested` arm-ит `clearAction`, второй выполняет clear, timeout
сбрасывает armed state. Это отдельная граница между Control и ViewModel.

`BankManagerViewModel` не содержит legacy `manager`/policy objects и не хранит
action callbacks. Он публикует только semantic feature state; сценарии действий
живут в `BankManagerController`, который получает ViewModel и root
`ConsolidatorViewModel`.
Для link groups ViewModel публикует `activeLink` и `selectionActive` отдельно;
Presenter использует `selectionActive` в edit mode и `activeLink` в обычном
режиме.

Bindings используют единый формат `{ source, read, write, map }`. `read`
преобразует source value для presentation, `write` выполняет обратное
преобразование перед записью, а `map` собирает сложный source в semantic
presentation value. Например, inverted active binding и telemetry mapping
выглядят так:

```js
active: bindPresentation(vm.compressor.bypass, {
    read: function (value) { return !value; },
    write: function (value) { return !value; }
}),
visualization: {
    source: vm.analyzer.compressorMeter,
    map: function (meter) {
        return {
            type: "level",
            peak: normalizeDb(meter.peakDb),
            smoothed: normalizeDb(meter.smoothedDb)
        };
    }
}
```

`DialPresenter` использует этот binding contract для всех application sources,
поэтому feature-specific инверсии и преобразования не попадают в generic
presenter. `activeIndex` и `displayIndex` являются interaction state presenter-а
и не binding-ятся к ViewModel.

`ButtonPresenter` и `SliderPresenter` используют ту же границу. Button публикует
`value`, optional `active`, `enabled`, `mode` и `label`; `value` является
пользовательским toggle state и определяет selected-визуализацию control, тогда
как `active` не участвует в toggle semantics. Slider публикует
normalized `value/minimum/maximum`, orientation, formatted display value и color.
Slider хранит physical range и physical step только во внутреннем mapping и
делает обратное преобразование в `setValue()`.

Telemetry также преобразуется на composition boundary, а не внутри dial:

```js
function levelVisualization(source) {
    return {
        source: source,
        map: function (meter) {
            return {
                type: "level",
                peak: normalizePresentationValue(meter.peakDb, -60, 0),
                smoothed: normalizePresentationValue(meter.smoothedDb, -60, 0)
            };
        }
    };
}

function saturationVisualization(source) {
    return {
        source: source,
        map: function (meter) {
            return {
                type: "saturation",
                value: Number(meter.percent) / 100,
                smoothed: Number(meter.smoothedPercent) / 100
            };
        }
    };
}
```

После mapping `DialPresenter` создаёт sparse type-specific presentation:
`{ type: "level", peak, smoothed }`, `{ type: "reduction", value }`, либо
`{ type: "relative", value }`. Он не знает, был ли source level, reduction или
saturation telemetry.

Для простого source mapping можно передать физическую telemetry напрямую с
явным range:

```js
visualization: {
    type: "level",
    peak: vm.analyzer.compressorMeterPeakDb,
    smoothed: vm.analyzer.compressorMeterSmoothedDb,
    range: { minimum: -60, maximum: 0 }
}
```

Presenter нормализует все visualization values в `0..1` до публикации
`DialPresentation`. Например, для gain reduction используется range `0..20`,
для процента distortion — `0..100`. Control получает только normalized values и
не знает физические единицы.

Для dial normalized `value`, `minimum`, `maximum` и `defaultValue` вычисляются
от physical range; `setValue()` выполняет обратное преобразование и делегирует
physical value в исходный writable ViewModel. Presenter actions включают value,
reset, active и active index intents. Bindings не изменяются после создания presenter;
visualization и ring color приходят из ViewModel sources либо static
configuration. Existing controls и renderers на этом этапе не изменяются и
presenters к ним не подключаются.

Если ViewModel не предоставляет настоящий default value, presentation содержит
`defaultValue: null`; reset не делает optimistic preview и ждёт authoritative
snapshot после `source.reset()`.

## Generic controls

`js/Controls/Dial/DialControl.js` — новый нейтральный Max JS control. Он
принимает `DialPresentation` snapshot и не знает о ViewModels, Client или
feature-specific actions. Control рисует rings/visualization и публикует через
outlet generic events: `valueChanged`, `reset`, `gestureBegan` и
`gestureEnded`. Presentation принадлежит Presenter и не мутируется control-ом;
во время drag control использует только локальный preview до прихода нового
snapshot. Legacy dial в `Max/` остаётся без изменений.

Во время active gesture control сохраняет локальный preview только для текущего
ring. Authoritative snapshots продолжают обновлять metadata, limits и
visualization, но не сбрасывают этот preview; он удаляется при
`gestureEnded`.

## Presenter boundary

Новый Presenter layer фиксирует однонаправленную границу:

```text
ViewModels
    │
binding/config
    ↓
DialPresenter
    ┌───────────────┴───────────────┐
    ↓                               ↑
presentation                    actions
    ↓                               ↑
DialControl ────────────────────────┘
```

Downward presentation включает `value`, `limits`, `display`, `visualization`,
`active`, `enabled` и `color`. Upward идут только user intents: `setValue`,
`reset`, `setActive`, `setActiveIndex` и `gestureBegan`/`gestureEnded`.

Presenter observable уведомляет только об изменении presentation snapshot.
Отдельные presenter events оставлены лишь для gesture lifecycle; `valueChanged`,
`reset` и `activeChanged` не эмитятся обратно, потому что authoritative result
приходит через ViewModel и следующий snapshot.

`DialPresenter` является reference implementation для нового Presenter layer:
physical `step` применяется только после denormalization, snapshots не
мутируются control-ом, bindings остаются неизменными после конструктора, а
`PresentationBinding` предоставляет общий `source` + `read`/`map`/`write`
contract. Дальнейшее расширение `DialControl` отложено до стабилизации этой
границы.

Published ring presentation содержит только UI data:

```js
{
    value: 0.63,
    minimum: 0.2,
    maximum: 0.9,
    display: { value: "-18.0 dB" },
    visualization: { type: "level", peak: 0.78, smoothed: 0.61 },
    color: null
}
```

Physical ranges, steps, logarithmic mapping и обратные transforms остаются во
внутренней configuration/mapping Presenter-а и не попадают в `DialPresentation`.

Mapping и display разделены:

```js
new SliderPresenter({
    value: vm.frequency,
    mapping: { type: "logarithmic" },
    display: { decimals: 0, suffix: " Hz" }
});
```

`mapping` описывает преобразование physical/UI values, а `display` — только
форматирование уже выбранного physical value.

### Runtime boundary

`DialPresentation` — локальный JS snapshot. Если Presenter и Control находятся
в разных Max JS contexts, snapshot нельзя передать через patch cord как object.
Поэтому `DialControl.applyPresentation(presentation)` является только
in-process seam и не считается transport API:

```text
DialPresenter
    ↓ snapshot
PresentationAdapter / encoder
    ↓ Max atoms/messages
DialControl
```

Будущий `DialControlAdapter` будет отвечать за кодирование presentation в
messages и декодирование control intents обратно. Headless Presenter не должен
знать о Max atoms, outlet или patch wiring.

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

`RegistryClient` — третий client поверх того же `NativeProtocolClient`. Он
хранит только последний registry snapshot, отбрасывает response с более
старой `revision` и предоставляет `get()`, `fetch()` и `subscribe()`.
`BankManagerViewModel` принимает этот client и строит semantic rows и link
groups из snapshot; Control по-прежнему получает только presentation.
Registry updates arrive as a small broadcast `registry_changed version revision`
notification;
the client then performs an explicit snapshot fetch, so no polling or event
replay is needed.

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
