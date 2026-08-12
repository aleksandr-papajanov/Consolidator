# Consolidator.Max

Новый домен Max-интерфейса Consolidator.

`externals/` содержит копии native Max-сборок, подготовленные для локальной
разработки Max-патчей. `js/` разделён на два слоя:

- `Clients/` — `ConsolidatorClient`, state/analysis clients и native transport;
- `ViewModels/` — UI projections без native paths и selectors.

Клиентский интерфейс работает поверх двух native-потоков:

- `state` — authoritative state protocol;
- `analysis` — global analysis view, spectrum/curves и telemetry.

Основной lifecycle:

```js
var client = new ConsolidatorClient("ui.main", send);
var vm = new ConsolidatorViewModel(client);

vm.initialize();
vm.selectBank(3);
vm.analyzer.show(instanceId, bankId);
```

`selectBank()` изменяет selected bank собственного instance. Analysis view
другого instance/bank выбирается отдельно через `analyzer.show()`.

Папка `Max/` в корне репозитория — legacy-источник контекста. Она больше не
является рабочим доменом и используется только для изучения старых патчей,
компонентов и элементов, которые могут быть перенесены сюда позже.

JS contract-test запускается командой `node tests/ClientTests.js` из этой
папки. Он проверяет control framing/cache и фильтрацию analysis frames по view.
