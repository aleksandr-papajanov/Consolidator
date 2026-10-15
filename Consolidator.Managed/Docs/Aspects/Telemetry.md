# DSP Telemetry

## Владение

Это transient latest-only канал Native -> Managed worker -> активный VST3 editor.
Он не является state tree и не участвует в grouping/history/persistence. Только
явное выключение Leveler может зафиксировать его slow gain обычной Managed transaction.

```text
Native audio-owned measurements -> DspTelemetryExchange
    -> DspTelemetryService -> dsp_telemetry / track_levels
    -> независимые native latest slots -> message-thread presentation clients
```

AudioRuntime публикует bounded POD после блока через odd/even sequence. Аудиопоток
не выделяет память, не вызывает Managed/UI и не форматирует protocol atoms.
Managed читает presence mask и payload между двумя чтениями sequence, с memory
barrier перед повторным чтением. Worker опрашивает источники каждые 33 мс.
Unregister удаляет borrowed exchange pointer под reader lock до освобождения Native storage.

## Измерения

| Shared value | Смысл |
| --- | --- |
| InputLevel / InputLevelFast | Slow input gain и fast correction, dB |
| InputRms | 300 мс main RMS после input gain, dBFS |
| OutputGain / OutputGainFast | Slow output gain и fast correction, dB |
| OutputRms | 300 мс main RMS после output gain, до limiter, dBFS |
| OutputLevel | 300 мс RMS финального выхода после limiter, dBFS |
| CompressorReduction | Peak применённой compression reduction за последний блок, dB |
| LimiterReduction | Peak применённого limiter attenuation за последний блок, dB |

Input/Output automatic gain присутствуют только когда Leveler включён и секция
не bypassed (при instance mute/bypass удерживаемый gain остаётся валидным).
Это также условие допустимости freeze через IGainStageTelemetry.
RMS секции присутствует в обоих режимах её обработки. Reduction положительна,
не включает compressor makeup/output gain и dry/wet. При instance mute/bypass
reduction публикуется нулевой. Это latest block peak, без накопления между UI polls.

## Focused wire frame

```text
dsp_telemetry version sourceInstanceId revision valueCount path value ...
```

Версия 1, bounded variable list до восьми пар. При выключенном Leveler публикуется
`gain_staging.input_target` / `gain_staging.output_target` с измеренным RMS. При
включённом публикуются соответствующие `input_level` / `output_level` и
`input_level_fast` / `output_level_fast`. Telemetry не создаёт state по этим путям.

Reduction передаётся по `compression.reduction_db` и
`gain_staging.limiter_reduction_db`. Это presentation-only paths без state writes.
GainTelemetryClient валидирует source, revision, размер, известные уникальные paths
и finite values; сохраняет последний frame, включая reduction. Индикаторы reduction
пока не подключены. Ручки получают отдельные RuntimeDialViewModel overlays.

Source следует focused bank, как analyzer. До готового snapshot/prepared audio
presentation ничего не применяет. Target/audio configuration transitions очищают
runtime overlay. При отсутствии активного viewer worker не создаёт protocol frames.

## Track levels

```text
track_levels version revision instanceCount instanceId outputLevelDb ...
```

Активному viewer отправляется complete latest snapshot финальных уровней всех
зарегистрированных инстанций независимо от Leveler flags. Native хранит только
последний pending frame каждого telemetry kind отдельно от FFT. TrackLevelClient
не меняет state tree. Bank Manager и graph track list обновляют level dot без
перестроения registry: -60..0 dBFS -> 0..1, степень 1.8 -> alpha.

## Freeze и UI

При обычной записи input_leveler/output_leveler=false Managed разрешает выбранную
группу, читает slow gain каждого target и фиксирует его вместе с flag. Нет валидной
telemetry — остаётся его committed gain. Native убирает fast overlay плавным ramp.
Сохранённые gains участвуют в обычных Undo/Redo и persistence. UI не отправляет
зависимых gain writes. Полный state contract: [GainStaging](GainStaging.md).
