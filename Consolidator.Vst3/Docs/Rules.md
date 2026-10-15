# Consolidator.Vst3 — правила

Основной Windows JUCE frontend и host adapter Consolidator.
Прикладное поведение устройства принадлежит Managed, обработка аудио — Native.

## Зона ответственности

Интерфейс получает Managed snapshots/notifications, обновляет ViewModels,
биндит их к JUCE controls и отправляет пользовательские intents обратно.
Локально допустимы geometry, focus, hover, selection представления, scale,
видимость кривых и временный drag preview до Managed acknowledgement.

Bindings связывают intent и подтверждённое состояние; ViewModels готовят
presentation. Контролы выполняют hit testing, жесты и рисование. Визуальный
расчёт кривой из принятых значений не является audio processing или state mutation.

Host adapter владеет JUCE lifecycle, audio buffers и Managed registration,
передаёт main/sidechain audio в существующий capture transport, вызывает общий
Native DSP и связывает host save/load с Managed persistence. Он не интерпретирует
сохранённое состояние и не публикует DSP-параметры от имени UI.

## Запрещённая ответственность

- Собственное прикладное state tree, defaults/ranges параметров или persistence.
- Выбор peers и распространение изменений по группе.
- Политика участия параметров в истории, зависимые записи и фиксация auto level.
- Команды, которые обходят Managed state/observers/history и меняют DSP напрямую.
- Собственные DSP-алгоритмы, FFT анализа основного устройства или аппроксиматор.

Начало/конец жеста — intents, а не frontend-политика истории.
Изменения контролов сохраняют accepted/preview разделение, финальное acknowledgement
жеста, suppression устаревших target replies, snapshot readiness и lifetime.
Переиспользовать shared controls и feature boundaries независимо от положения
контрола на экране. Действующий контракт — [Gestures](Aspects/Gestures.md),
[SnapshotAssembly](Aspects/SnapshotAssembly.md) и [FrontendComposition](Aspects/FrontendComposition.md).
Неготовый DSP не оправдывает реализацию его state-поведения в C++ UI.
JUCE APVTS/automation parameter tree для основного устройства не вводится.
Не путать этот проект с отдельными исследовательскими `ModelPlugins.Vst3`.


Общие требования — [Rules](../../Docs/Rules.md). Техническое устройство проекта — [Architecture](Architecture.md).
