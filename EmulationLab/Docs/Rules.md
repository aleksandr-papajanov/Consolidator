# Правила ручного исследования

Стенд используется по явному запросу на исследование/прослушивание;
сборка отдельно требует разрешения. Общие ограничения — [AGENTS.md](../../AGENTS.md),
граница DSP-тестов — [Native DSP rules](../../Consolidator.Native/Docs/Rules.md).

Не превращать renders, compare, validate, fit или CPU-измерения в автоматические
DSP regression suites, self-test gates или обязательную проверку каждой правки.
Наличие команды в CLI не является разрешением на её запуск.

До интерпретации результата фиксировать reference/candidate, реальные значения
параметров/readback, sample rate, block size, conditions и способ reset/prepare.
Не смешивать datasets с разными schema/signatures и measurement windows.
Методы и ограничения измерений описаны в [MeasurementMethods](Aspects/MeasurementMethods.md).

Принятые coefficients/defaults живут в общем Native-коде. Model adapter не
содержит альтернативного алгоритма; основной VST3 не перенимает исследовательское
host parameter/state устройство этих adapters.

Сырые входы с происхождением хранить в RawAudio. Краткие принятые выводы — в
Native Docs/Aspects, использование конкретного опыта — в его manifest/README.
Временные reports, processed WAV и одноразовые fitting scripts удалять после
завершения исследования. Артефакты не подменяют production documentation.
