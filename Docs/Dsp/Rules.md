# Правила DSP

## Реализация

У каждого процессора — один Native исходник, свой документ алгоритма и собственные runtime states.
Законченные Cla76, Cla3a, TransientShaper, Air и Thick находятся в
Consolidator.Native/External/Dsp/Processors: папка процессора содержит .h/.cpp,
README.md и Findings.md. Namespace — consolidator::max::dsp::processors;
общие DSP-примитивы и runtime composites остаются в родительской Dsp.
VST3-адаптер компилирует и вызывает этот же Native процессор. В оболочке разрешены
только host parameters/state, bypass и преобразование буферов; копии алгоритмов,
отдельные model-only DSP реализации и независимые DSP defaults запрещены.
Непонятный код сначала исследовать по документации. Память, коэффициенты и bounded
storage подготавливаются до аудиопотока. В Process запрещены allocation, locks,
файловый I/O, logging, offline analysis/optimization и Managed calls.
Не добавлять fallback, legacy APIs, альтернативные реализации и неподтверждённые
защиты. Неудачная гипотеза удаляется; полезные выводы кратко сохраняются рядом с алгоритмом.

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
