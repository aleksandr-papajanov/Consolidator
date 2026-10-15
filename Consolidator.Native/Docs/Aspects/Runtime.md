# DSP Runtime

## Владение и слои

Основной channel strip находится в `Consolidator.Native/Audio`, namespace
`consolidator::audio`. Он не зависит от Max/JUCE и получает заимствованные buffers.

| Папка / namespace | Ответственность |
| --- | --- |
| `Core` / `audio` | AudioBuffer, DetectorBuffer, ProcessingContext и физические FilterParameters |
| `Primitives` / `audio::primitives` | Ramps, gain, width, reusable filters/cascades, RMS meter и математические операции |
| `Algorithms` / `audio::algorithms` | Leveler, Saturation, TransientShaper, CLA/SSL, Thick, Air и Limiter |
| `Devices` / `audio::devices` | Параметры секции, detector EQ, композиция и bypass/parallel transitions |
| `Chain` / `audio` | ConsolidatorChain: владение устройствами и фиксированный порядок вызовов |
| `Runtime` / `audio::runtime` | Binding snapshot, lifecycle, проверка capacity, audibility и telemetry |

`Transport` владеет Managed bridge, общим POD ABI, snapshot exchange и
borrowed-data lifetime. Только Runtime связывает transport snapshot с Native
параметрами; algorithms и devices не читают wire snapshot, offsets или transport policy.

Исследовательские `ModelPlugins.Vst3` adapters используют актуальные реализации
из `Audio/Algorithms`. Их независимые host-параметры не являются частью основного
runtime. Прежний `External` код не участвует в сборках или runtime.

## Порядок сигнала

```text
Input: Width -> GainStage
Saturation: Detector EQ -> saturation -> output gain
Shaper: собственный HP20 detector -> актуальный TransientShaper
Compression: Detector EQ -> выбранная SSL / 76 / 3A / 2A модель -> output
Equalizer: 7 банков x 7 фильтров последовательно
Polish: Thick -> Air
Output: GainStage -> sample-peak Limiter
Final output RMS -> track-level telemetry
```

Gain Staging — один Managed domain, но Input и Output физически находятся на
разных концах цепочки. Каждый GainStage имеет независимые DetectorEqualizer,
Leveler и RMS meter. Width принадлежит только Input.

DetectorEqualizer копирует текущий вход устройства в подготовленные private
buffers и применяет LowShelf/Bell. Возвращаемый read-only DetectorBuffer живёт
до следующего Capture этого экземпляра. Детектор никогда не подмешивается в main.
Input capture происходит после Width. Compressor capture — после Shaper.
Output capture — после Polish, до output gain и limiter.

CompressorAlgorithm совпадает с Managed Type: 0 SSL, 1 CLA76, 2 CLA3A, 3 CLA2A.
Профили настроек принадлежат Managed. Native получает уже выбранные значения.
Все модели Prepare выполняют заранее, аудио обрабатывает только выбранная.
При смене модели её история и ramps сбрасываются к текущим controls. Переключение
алгоритма резкое; click-free morph не заявляется. Сохраняются Normal control laws
и статическая model makeup. Output ручной как в основном strip, так и в
исследовательском адаптере; динамической выходной компенсации нет.
Parallel смешивает dry/wet 50/50 после model output; выключенный Parallel даёт 100% wet.

## GainStage

При выключенном Leveler ручной Level задаёт gain. Target read-only и показывает
300 мс linked RMS после gain. Это измерение main, без detector EQ; в Output оно
снимается до limiter. Final track RMS снимается после limiter отдельным meter.

При включённом Leveler Target задаёт желаемый K-взвешенный detector level, Level read-only и
показывает slow gain. Fast correction остаётся отдельной telemetry. Контроллер
использует прежние slow/fast windows, gate и gain laws: [Leveler](LevelerProcessor.md).
KWeighting работает после detector EQ только в управляющей ветке. Времена
усреднения и регулирования сохранены; это не LUFS meter. Измеренный main RMS
не обязан совпадать с K-взвешенным Target.

При отключении Leveler Managed фиксирует slow gain каждого участника группы
отдельно. Native плавно снимает fast overlay. State/history/persistence не
записываются из аудиопотока: [GainStaging](../../../Consolidator.Managed/Docs/Aspects/GainStaging.md).

## Lifecycle и переходы

AudioRuntime Prepare выполняется при остановленном аудио с положительным finite
sample rate. Нулевая capacity означает 4096 samples. Все device/detector/branch
buffers выделяются здесь. Повторный Prepare пересчитывает коэффициенты под новый
sample rate и сбрасывает истории. Первое принятое состояние задаёт текущие controls;
последующие snapshots меняют targets, сохраняя audio-owned histories.

ProcessBlock проверяет capacity и наличие хотя бы одного канала до вызовов DSP.
Невалидный/пустой блок не обрабатывается. Mono допускает один null channel.
Не слышимая инстанция выдаёт нули; instance bypass сохраняет вход. В обоих случаях
истории устройств заморожены. Финальный meter продолжает измерять фактический выход.

Устройства владеют WetTransition: 10 мс bypass/activity crossfade с подготовленной
wet-копией. Settled 100% wet работает прямо на main. После завершения перехода в
ноль история сбрасывается один раз. Saturation/Shaper/Polish используют neutral
activity; Compressor продолжает обновлять свои histories при Reduction=0, но
Normal не выполняет встроенные аудиофильтры окраски.
EQ использует собственные устойчивые crossfade двух каскадов вместо общей wet-копии.

Непрерывные controls сглаживаются внутри потребляющих их algorithms/primitives.
Binding не сглаживает их повторно. Filter controls меняют coefficients через
10 мс output crossfade, без интерполяции сырых коэффициентов.

Весь Process hot path не выделяет память, не использует locks, logging, I/O,
Managed calls, registry lookup или host API. Объекты принадлежат одному audio owner.

## Telemetry

AudioRuntime публикует bounded POD через существующий odd/even sequence exchange.
Input/Output slow gain доступны для freeze только когда соответствующий Leveler
включён и секция не bypassed. При instance mute/bypass сохранённый slow gain остаётся
валидным для freeze. RMS доступен в обоих режимах обработки.
Native не выбирает presentation target и не форматирует protocol frames.

Compression reduction и Limiter reduction — положительное peak ослабление в dB
за последний обработанный блок. Compression измеряется до makeup/output и dry/wet;
Limiter — по реально применённому gain. В instance mute/bypass значения reduction
равны нулю. Передача latest-only; это не накопленный peak за UI polling interval.

Managed и VST3-клиент принимают reduction без визуального индикатора.
Wire/lifetime: [Telemetry](../../../Consolidator.Managed/Docs/Aspects/Telemetry.md).
