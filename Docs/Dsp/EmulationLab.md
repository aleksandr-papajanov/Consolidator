# EmulationLab: ручные исследования DSP

Offline VST3 host, измеритель и оптимизатор. Запускается для исследования аудио только
по явному запросу; сборка требует разрешения. DSP-автотестов и self-test workflow нет:
[правила](Rules.md). Исполняемый файл:
EmulationLab/build/EmulationLab_artefacts/Release/EmulationLab.exe.

## Быстрый рендер из корня репозитория

Пример ниже предполагает собственный manifest для Transient с condition
`steady-neutral`. После очистки старый transient-master manifest отсутствует:
сначала создать его на основе Experiments/example/manifest.json и настроить плагины.

```powershell
$lab = 'EmulationLab/build/EmulationLab_artefacts/Release/EmulationLab.exe'
$manifest = 'EmulationLab/Experiments/transient-master/manifest.json'
& $lab render-wav reference $manifest steady-neutral EmulationLab/RawAudio/Recordings/tr4-raw.wav reference.wav
& $lab render-wav candidate $manifest steady-neutral EmulationLab/RawAudio/Recordings/tr4-raw.wav candidate.wav 'Attack=0.75' 'Sustain=0.5'
& $lab transient-envelope EmulationLab/RawAudio/Recordings/tr4-raw.wav reference.wav candidate.wav envelope.json
```

Числа overrides — normalized VST3 values, не отображаемые значения ручки.
Signed Attack/Sustain:−1→0, 0→0.5, +1→1. Output Gain−12…12 дБ:0.5 нейтрально.
У текущего candidate нет Limit-параметра; reference Limit1 задан условиями manifest.
render-wav сохраняет rate/stereo, пишет float32 WAV, не нормализует и не вычитает latency.
Существующий output не перезаписывается. Для transient-envelope нужны одинаковые
sample rate, длина и выравнивание файлов. Raw inputs: [каталог](../../EmulationLab/RawAudio/README.md).

## Команды

```text
plugin describe <bundle> [query]
plugin parameter-manifest <bundle> <output.json> [query]
plugin save-state <bundle> <output.state> <query|-> [Name=Text ...]
manifest validate <manifest>
capture <reference|candidate> <manifest> <output.json>
split <manifest> <reference-full.json>
compare <manifest> <reference.json> <candidate.json> <report.json>
validate <manifest> <reference.json> <candidate.json> <max-loss>
fit <manifest> <output.json> [resume.json]
diagnose <reference|candidate> <manifest> <condition> <output.json> [fit.json]
```

Для нового устройства взять Experiments/example/manifest.json: указать paths/query,
rates, block size, conditions, probes, objectives и bounded tuning stages.
Относительные пути считаются от папки manifest. Numeric conditions — normalized0…1;
строки — display text. Readback metadata фиксирует реально применённые значения.
Condition control не может одновременно быть optimizer parameter.

capture пишет dataset/environment metadata. Schema/signature должны совпадать:
нельзя смешивать grids и measurement windows разных опытов. split делит целые
stateful sequences. fit запускает staged bounded NLopt search с checkpoints,
protected probes/features и отдельным dataset для сравнения. Это ручной исследовательский
workflow, не test suite и не автоматическое изменение production defaults.
Resume seed не восстанавливает population NLopt. Tuning применяется до Prepare.

## Что учитывать

- Проверить actual control readback и neutral baseline; rack reference измерять с solo
  нужного модуля. Sidechain HP и output HP имеют разные роли.
- Форму и громкость сравнивать отдельно: удалить средний dB offset по файлу/удару
  для shape, отдельно показать RMS/gain bias/peak. Один scalar score скрывает регрессии.
- Attack effect сравнивать с Attack0 при одинаковых Sustain/Limit.
- Всегда указывать окно1/2/5/20 мс и привязку к dry onset. Power release и amplitude
  release не равны. Carrier zero crossings не описывают gain envelope.
- Проверять cold start, quiet/scaled copies, слабый удар после сильного, stationary
  tone и down-step. Prepare очищает histories; continuous render их сохраняет.
- Settled sine: минимум12 preroll periods, 20 measured periods, Hann projection и
  significance floor. Leakage короткого окна нельзя принимать за гармоники.
- capture/probes учитывают reported latency; render-wav — нет. В тишине gainDb=null,
  а не measured0 дБ. Не смешивать разные методы выравнивания.
- Из complex baseline делить fundamental; harmonic powers вычитать по порядкам и
  заново рассчитывать THD. Paired output differences не доказывают внутреннюю wet-ветку.
- LinearAnalysis динамического плагина зависит от уровня и истории. Sine response
  не доказывает внутреннюю схему фильтров, duck или compressor reference.
- Whole-process CPU включает host/I/O/timer noise. Использовать длинные одинаковые
  renders, alternating order и bypass subtraction; это не callback timing Plugin Doctor.
- Загруженный VST3 нельзя заменить: сохранить проект, закрыть host, установить готовую
  сборку. После изменения parameter ranges использовать свежий экземпляр; старые
  normalized automation могли иметь другое значение. Не вводить aliases/migration.
- После выбора алгоритма оставить defaults в коде и короткие выводы в документе
  процессора; удалить временные отчёты, fitting scripts и processed WAV.

Reusable scripts: EmulationLab/Scripts/CompareTailBands.ps1 сравнивает полосы/окна,
RenderBandpass.ps1 фильтрует aligned stereo для прослушивания. Это offline инструменты,
не дополнительная realtime-топология.

## Локальные артефакты

`.gitignore` исключает Measurements, diagnostics, results, аудио, сборки и содержимое
EmulationLab/Experiments. В Experiments сохраняются manifest.json, README.md и
исходники/скрипты. RawAudio остаётся локальным вместе с catalog.json; его README
хранится в репозитории. Результаты экспериментов не добавлять в исходный код или Docs.
Игнорирование не удаляет файлы и не снимает уже отслеживаемые файлы с учёта Git.
