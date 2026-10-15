# Native — правила

## Реализация

У каждого процессора — один Native исходник, свой документ алгоритма и собственные runtime states.
Законченные Cla76, Cla3a, Cla2a, SslChannel, TransientShaper, Air и Thick находятся в
Consolidator.Native/External/Dsp/Processors: папка процессора содержит .h/.cpp.
Документ каждой модели и отдельный файл наблюдений находятся в [Aspects](Aspects/ProcessorModels.md).
Namespace — consolidator::max::dsp::processors;
общие DSP-примитивы и runtime composites остаются в родительской Dsp.
ModelPlugins.Vst3 компилирует и вызывает тот же Native процессор; его host
parameters/state описаны в [исследовательском adapter contract](../../EmulationLab/Docs/Aspects/ModelPluginsVst3.md).
Основной Consolidator.Vst3 имеет другую границу: только Managed presentation/
intents и host adapter, см. [VST3 Rules](../../Consolidator.Vst3/Docs/Rules.md).
Копии алгоритмов, model-only DSP и независимые DSP defaults запрещены.
Непонятный код сначала исследовать по документации. Память, коэффициенты и bounded
storage подготавливаются до аудиопотока. В Process запрещены allocation, locks,
файловый I/O, logging, offline analysis/optimization и Managed calls.
Не добавлять fallback, legacy APIs, альтернативные реализации и неподтверждённые
защиты. Неудачная гипотеза удаляется; полезные выводы кратко сохраняются в соответствующем Aspects/<Model>Findings.md.

## Оптимизация и численная устойчивость

Оптимизация сохраняет согласованный алгоритм и характер обработки. Отделять
математически эквивалентное упрощение от приближения, изменяющего результат.
Дорогие расчёты выносить на минимально необходимую частоту пересчёта по реальным
зависимостям; неизменившиеся controls не требуют повторного расчёта coefficients.
Разделять Prepare/storage, обновление control laws и sample processing.
Realtime-ограничения относятся ко всему вызываемому hot path, включая helpers,
а не только к телу Process.

Для управляющих таблиц и многоступенчатых интерполяций предпочитать компактную
функцию, если её точности достаточно в требуемом диапазоне. Учитывать endpoints,
непрерывность, монотонность, допустимый знак/диапазон, знаменатели и поведение у
нуля. Не заменять точный закон приближением и не снижать control rate автоматически.
Подбор и сравнение звука выполняются только в разрешённом исследовательском scope.

Интерполяция control law не заменяет временное сглаживание. Сохранять ramps,
устойчивые filter transitions, порядок стадий и histories, включая не слышимые
сейчас ветви, если они нужны при последующем включении. Не вводить смену precision,
approximate math или oversampling как побочный эффект структурной оптимизации.
Снижение количества операций само по себе не доказывает ускорение CPU.
При neutral/bypass/endpoint оптимизации сохранять нужное продвижение histories,
channel isolation/linking, detector taps и порядок update/sample. Конец ramp
внутри блока должен достигать точного target; объединение проходов не должно
менять эти зависимости. Не интерполировать сырые biquad coefficients вместо
существующего устойчивого перехода. При рефакторинге прослеживать Prepare/Process/
teardown и затронутые call sites; требуемое изменение звука отделять от изменения
структуры с сохранением поведения.
Текущие механизмы — [Runtime](Aspects/Runtime.md) и [Implementation](Aspects/Implementation.md).

## DSP-автотесты запрещены

Не создавать и не запускать unit/regression/golden/property/benchmark test suites
для аудиоалгоритмов, DSP-примитивов и runtime graph. Не добавлять DSP CTest/self-test
gates и обязательное автоматическое сравнение звучания.
Это политика проекта, не утверждение о недетерминированности: при одинаковом входе,
состоянии, параметрах и окружении алгоритм детерминирован. Субъективна оценка звучания.

Тесты ABI, protocol, persistence, Managed lifecycle и транспорта — отдельный scope.
Передача DSP snapshot не превращает проверку ABI в тест аудиоалгоритма.
Статический обзор source, ownership/realtime и ссылок не является DSP-автотестом.

## Ручные исследования

По явному запросу пользователя разрешены прослушивание, Plugin Doctor, рендеры,
подбор и сравнение через EmulationLab, измерение CPU. Они не становятся автоматическими
тестами или обязательным quality gate. Сборка также требует разрешения.
Raw inputs сохраняются в EmulationLab/RawAudio; временные результаты удаляются после
завершения исследования. Defaults принадлежат исходному коду. Улучшение одного metric
не оправдывает ухудшение звука.

## Граница состояния

Native принимает опубликованный Managed DSP snapshot. Коэффициенты, histories,
envelopes, ramps и buffers — локальное состояние исполнения. Banks/groups,
application defaults/ranges, зависимые edits, Undo/Redo и persistence принадлежат
Managed; Native transport не интерпретирует смысл команд и state paths.
Host API не попадает в DSP. Архитектура — [Architecture](Architecture.md),
общие ограничения — [Rules](../../Docs/Rules.md).
