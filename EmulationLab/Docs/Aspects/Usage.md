# EmulationLab: ручные исследования DSP

Offline VST3 host, измеритель и оптимизатор. Запускается для исследования аудио только
по явному запросу; сборка требует разрешения. DSP-автотестов и self-test workflow нет:
[правила](../../../Consolidator.Native/Docs/Rules.md). Исполняемый файл:
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
Нельзя считать normalized value процентом шкалы референса: Waves CLA-2A,
например, отображает Gain 40.00 при normalized 0.535 и Peak Reduction 46.00
при normalized 0.598. Для выбора по числам интерфейса передавать текстовые
overrides. `plugin parameter-scale` сохраняет по 1001 паре normalized/text
для выбранных параметров, без рендера аудио, с одним сканированием bundle.
Signed Attack/Sustain:−1→0, 0→0.5, +1→1. Output Gain−12…12 дБ:0.5 нейтрально.
У текущего candidate нет Limit-параметра; reference Limit1 задан условиями manifest.
render-wav сохраняет rate/stereo, пишет float32 WAV, не нормализует и не вычитает latency.
`render-grid <reference|candidate> <manifest> <input.wav> <output-directory>`
рендерит тот же вход для всех conditions в `<condition>.wav`. Каждый condition
получает новый экземпляр плагина и Reprepare; только сканирование bundle разделено
через один Vst3Host. Overrides отсутствуют: параметры берутся из manifest.
Condition IDs должны быть именами файлов без компонентов пути. Существующие WAV
не перезаписываются. Это ручное исследование, не DSP test suite.
Рядом с WAV сохраняется `.parameters.json`: идентификатор реального плагина,
показания и normalized values всех параметров после применения controls и Reprepare.
Этот readback позволяет обнаружить неверную шкалу или несоответствие фактических
настроек манифесту до интерпретации аудиоизмерений.
Существующий output не перезаписывается. Для transient-envelope нужны одинаковые
sample rate, длина и выравнивание файлов. Raw inputs: [каталог](RawAudio.md).

## Команды

```text
plugin describe <bundle> [query]
plugin parameter-manifest <bundle> <output.json> [query]
plugin parameter-scale <bundle> <output.json> <query|-> <name> [name ...]
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


Методы: [MeasurementMethods](MeasurementMethods.md). Артефакты: [ExperimentArtifacts](ExperimentArtifacts.md). Discovery: [HostCaching](HostCaching.md).
