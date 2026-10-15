# DSP implementation structure

## Основной DSP

Папки, namespaces, data flow и lifecycle определены в [Runtime](Runtime.md).
Основной VST3 host вызывает `Audio/Runtime/AudioRuntime`, который хранит
один `ConsolidatorChain`. Цепочка содержит конкретные устройства без registry,
heap-owned списка или общего виртуального DspProcessor.

У устройства свои параметры, дочерние algorithms и runtime histories. У метода
Process явные audio/detector arguments. Заимствованный DetectorBuffer не меняет
маршрут следующих устройств. Единственная привязка wire snapshot к device parameters
находится в SnapshotBinding. Полный snapshot не проходит в algorithms/primitives.

Небольшие per-sample primitives и шаблон WetTransition находятся в headers.
Остальная реализация находится в .cpp. Композиция не вводит дополнительные
allocations или форматирование на audio thread.

## Алгоритмы

Saturation, TransientShaper, Thick, Air, Leveler и четыре compressor models
реализованы в `Audio/Algorithms`. Модели компрессии получают отдельный detector
buffer; telemetry снимается с применённой reduction до makeup и output. Изменение
порядка Shaper/Compression и включение Output Leveler являются согласованными
изменениями поведения.

Saturation сохраняет рационализированный ADAA residual, startup-weighted detector,
комплементарные 250/2500 Гц crossovers и продвижение неслышимых histories.
Выходной динамический Auto Gain удалён; ручной Output остаётся в устройстве. Thick
сохраняет конечный harmonic bank, body filters, upward dynamics и ceiling. Air
сохраняет contour и cubic harmonic path. TransientShaper сохраняет signed amounts,
собственный HP20, HP10 output и обе постоянные точки ограничения.

BiquadCascade готовит старый/новый paths и crossfade outputs. Повторный Prepare
перекомпилирует сохранённые параметры под новый sample rate, даже если controls
не изменились. Limiter сохраняет immediate downward ceiling и 80 мс release;
telemetry log вычисляется один раз на блок, а не для каждого sample.

Исследовательские adapters компилируют те же Native `.cpp` files из `Audio`.
Старая External-копия не входит в проект и не является альтернативным source path.

Source review, сборка и ручное прослушивание имеют разные границы доказательств.
DSP-автотесты запрещены [правилами](../Rules.md).

## Фильтры и измерение

Общие filter designs, состояния и обработка находятся в Primitives/Filters.
BiquadCascade компонует BiquadFilter и отвечает за EQ transitions. OnePole — TPT
секции; FirstOrderLowPass — экспоненциальный crossover Saturation, FirstOrderHighPass
— отдельная звуковая HP-секция. Их законы различаются и не подменяются друг другом.

Leveler использует KWeighting после своего detector EQ, с прежними slow/fast/gate
постоянными. Main RMS meters остаются невзвешенными. Компенсация компрессоров
по формулам находится в private CalculateMakeupGain каждой модели.
