# Consolidator.Max

Новый домен Max-интерфейса Consolidator.

`externals/` содержит копии native Max-сборок, подготовленные для локальной
разработки Max-патчей. `js/` разделён на два слоя:

- `Clients/` — `ConsolidatorClient`, state/analysis/registry clients и native transport;
- `ViewModels/` — UI projections без native paths и selectors;
- `Controls/` — новые generic Max JS controls, работающие с presentation
  snapshots.

Клиентский интерфейс работает поверх трёх native-потоков/границ:

- `state` — authoritative state protocol;
- `analysis` — global analysis view, spectrum, EQ/detector curves и telemetry.
- `registry` — global instance/bank/group read-model.

Основной lifecycle:

```js
var client = new ConsolidatorClient("ui.main", send);
var vm = new ConsolidatorViewModel(client);

vm.initialize();
vm.selectBank(3);
vm.analyzer.show(instanceId, bankId);
```

Имя instance является обычным authoritative state path `label`. Max-side
metadata writes используют тот же `StateClient`, поэтому переименование
проходит через Core `StateStore`, а не через отдельное ad-hoc событие. В Core
`InstanceCoordinator` поддерживает `RegistryState` как read-model всех
instance, bank/group membership, selected bank и `revision`; этот snapshot
является источником для registry transport/client. На JS стороне это
`client.registry` с latest-value API `get()`, `fetch()` и `subscribe()`.

`selectBank()` изменяет selected bank собственного instance. Analysis view
другого instance/bank выбирается отдельно через `analyzer.show()`.

Presenters
-----------

`AnalyzerPresenter` is the single generic presentation layer for equalizer and
detector modes. Feature controllers provide its sources and capabilities;
`AnalyzerControl` and `AnalyzerRenderer` only handle generic intents and
pixels. Axis ranges are supplied by feature controllers through
`frequencyRange` and `gainRange`; detector mode is not coupled to the EQ
vertical range. Detector curves are exposed by `AnalyzerViewModel` under
separate compressor and saturator observables. `AnalyzerPresenter` converts
frequency-response dB points into normalized y coordinates. Main/reference
spectra use their own configured `spectrumRange` (normally `-120..0 dB`),
while difference and response curves use the configured gain range. The
renderer does not interpret dB ranges.

Presenters получают только ViewModels и не зависят от Max API, controls или
renderers. `DialPresenter` композирует несколько state/analysis sources,
преобразует physical values в normalized ring values и предоставляет действия
`setValue()`/`resetValue()`/`setActive()`/`setActiveIndex()`; отдельные events
используются только для gesture lifecycle.
Visualization и ring colors приходят только из bindings или static
configuration. `ButtonPresenter`
и `SliderPresenter` используют тот же lifecycle и binding contract; Slider
публикует normalized range, а physical mapping/step оставляет внутри Presenter.
`BankManagerPresenter` использует тот же pattern для feature-level списка:
rows, focus и links публикуются как UI snapshot, а selection/action
возвращаются как generic intents.
Он принимает только semantic `BankManagerViewModel` и не адаптирует legacy
manager. Его presentation включает semantic `rows[].banks[]`, `linkGroups`,
`linkEditing`, `editAction` и `clearAction`; Control не вычисляет visibility,
link membership, opacity или enabled state. `BankManagerController` отдельно
обрабатывает selection, link editing, group actions, clear confirmation и writes.

```js
var compressorThresholdDial = new DialPresenter({
    rings: [{
        value: vm.compressor.threshold,
        display: { decimals: 1, suffix: " dB" }
    }],
    active: bindPresentation(vm.compressor.bypass, {
        read: function (value) { return !value; },
        write: function (value) { return !value; }
    })
});

var compressorBypassButton = new ButtonPresenter({
    value: vm.compressor.bypass
});

```

Каждый presenter публикует presentation snapshot через `subscribe()` и
освобождает все подписки через `destroy()`.

Новый нейтральный dial находится в `js/Controls/Dial/DialControl.js`. Он
принимает `DialPresentation` через `presentation()`/`applyPresentation()`,
рисует rings и generic visualization, а через outlet выдаёт только generic
events (`valueChanged`, `reset`, `gestureBegan`, `gestureEnded`). Старый
`Max/.../DialControl.js` этим control не заменяется и не изменяется.
Передача object snapshot между разными Max JS contexts пока не является частью
transport contract; для этого позже появится отдельный adapter/encoder.

Новые generic controls находятся в `js/Controls/Dial`, `js/Controls/Button`,
`js/Controls/Slider` и `js/Controls/BankManager`. Они принимают presentation snapshots через in-process
`applyPresentation()` seam, не знают о ViewModels и выдают только generic
intent messages.

Папка `Max/` в корне репозитория — legacy-источник контекста. Она больше не
является рабочим доменом и используется только для изучения старых патчей,
компонентов и элементов, которые могут быть перенесены сюда позже.

JS contract-test запускается командой `node tests/ClientTests.js` из этой
папки. Он проверяет state framing/cache, фильтрацию analysis frames по view,
registry multipart snapshots, `registry_changed` version framing, latest-value
fetch semantics, построение BankManager из registry snapshot и orchestration
`BankManagerController` для local/remote/link-edit selection и clear
confirmation.
