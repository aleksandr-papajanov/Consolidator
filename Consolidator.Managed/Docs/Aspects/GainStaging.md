# Gain Staging state and DSP projection

Gain Staging — единый Managed domain `gain_staging`. Его физические Input и Output
находятся до и после остальных устройств: [Native Runtime](../../../Consolidator.Native/Docs/Aspects/Runtime.md).

## Состояние

ManagedStateBuilder создаёт GainStagingState через обычный StateValueFactory с
instance ownership, connected edit scope, peers, history, reset и DSP publication.

| Relative leaf | Type | Default | Physical range |
| --- | --- | --- | --- |
| input_level | float, dB | 0 | -36..36 |
| input_target | float, dB | -18 | -36..0 |
| input_leveler | bool | false | boolean |
| output_level | float, dB | 0 | -36..36 |
| output_target | float, dB | -1 | -36..0 |
| output_leveler | bool | false | boolean |
| limit | float, dB | -1 | -36..0 |
| width | float | 1 | 0..1 |
| bypass | bool | false | boolean |

Независимые detectors: `gain_staging.input_detector.filter.*` и
`gain_staging.output_detector.filter.*`. Оба используют общий LowShelf/Bell catalog,
но имеют отдельные StateValues, peer/history/reset/persistence и snapshot slots.
Путь `gain_staging.detector` отсутствует. Числовые controls и Leveler flags участвуют
в history; Bypass использует обычную processor-control policy без history.

## Level / Target

Механика одинакова для input и output:

- Leveler выключен: Level редактируется, Target read-only и показывает измеренный
  сглаженный main RMS после gain. В Output измерение находится до limiter.
- Leveler включён: Target редактируется, Level read-only и показывает slow gain.
  Fast correction передаётся отдельно и не меняет положение Level.

GainStagingEdits проверяет режим каждого участника группы. Запись заблокированного
параметра отклоняется без частичного commit. В одной batch можно переключить режим
и записать ставший доступным параметр. Telemetry overlays не меняют committed state.

При выключении Leveler GainStagingEdits подготавливает slow gain каждого включённого
участника из его собственной telemetry в той же history transaction, что и flag.
Без валидного измерения остаётся прежний manual gain. Явная запись Level в batch
имеет приоритет. Reset отдельного Leveler flag делает такой же freeze; reset всей
секции возвращает defaults. Native плавно снимает fast overlay после отключения.

Width действует первым, только в Input. Оба bypass markers происходят от общего
Gain Staging Bypass. Limiter всегда включён в активной Output секции, его потолок
задаёт Limit. Output Target принадлежит выходному Leveler и не заменяет Limit.

Управляющий сигнал проходит detector EQ и KWeighting перед существующими slow/fast
followers Leveler. Telemetry main RMS остаётся невзвешенной. Автоматический Target
описывает K-взвешенный detector level и не обязан совпадать с main RMS meter.
Это не стандартный LUFS meter: времена, gate и усреднение каналов принадлежат
контроллеру Leveler. Native-контракт: [Leveler](../../../Consolidator.Native/Docs/Aspects/LevelerProcessor.md).

## Snapshot и persistence

Output detector добавлен в DspSnapshot двумя FilterSnapshot в offset 1508.
Размер DspSnapshot — 1556 bytes, DspTelemetryExchange — 44, DspStateExchange — 4720.
Exchange publishedIndex/consumerIndex/telemetry: 4668/4672/4676. Layout C# и C++
совпадает; snapshot не содержит defaults или pointers.

Persistence schema 9 хранит оба detectors как inputDetector/outputDetector вместе
со всеми controls. Restore требует все поля, проверяет ranges/shape и применяет
один baseline transaction. Предыдущие schemas не декодируются.

UI: [ChannelStrip](../../../Consolidator.Vst3/Docs/Aspects/ChannelStrip.md).
Telemetry: [Telemetry](Telemetry.md). Общий transport: [DspStateExchange](../../../Docs/Aspects/DspStateExchange.md).
