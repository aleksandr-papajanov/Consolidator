# CompressorProcessor

[Исходник](../../Consolidator.Native/External/Dsp/CompressorProcessor.cpp).

Wet-путь: detector EQ → [Compression](CompressionProcessor.md)/makeup →
[TransientShaperProcessor](../../Consolidator.Native/External/Dsp/Processors/TransientShaper/README.md) с собственным HP20 detector →
HP10 → Compressor Output Gain → постоянный sample clamp.

TransientShaperProcessor реализует последние этапы; тот же исходник используется VST3.
Повторного копирования detector EQ после compression нет. Detector EQ Compressor
управляет компрессией, а маски транзиент-шейпера получают её выход до собственного gain.

Attack/Sustain −1…+1. Маски и фильтры продолжают обновляться при нулевых ручках,
пока Compressor обрабатывает wet-ветку. Обе точки ограничения transient алгоритма
постоянны и не зависят от отдельного Limiter в Output графа.

Графовый ParallelStage: 50/50 dry/wet при Parallel, 100% wet без него, 10 мс переход.
Output Gain и финальный transient clamp находятся до смешивания. После полного
перехода в bypass граф сбрасывает состояние всего Compressor.
