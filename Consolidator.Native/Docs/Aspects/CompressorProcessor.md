# CompressorDevice

[Исходник](../../Audio/Devices/CompressorDevice.cpp).

Собственный DetectorEqualizer -> выбранная модель SSL / CLA76 / CLA3A / CLA2A.
Detector EQ не меняет main. Модель сохраняет свои detector/timing/reduction/colour
laws и статическую Normal makeup; затем применяется Output.

Shaper — отдельное предыдущее устройство и не является частью compressor wet path.
Устройства независимо bypassable. Источником параметров модели служат выбранные
Managed profiles: [Compression](../../../Consolidator.Managed/Docs/Aspects/Compression.md).

CompressorDevice владеет 10 мс WetTransition. Parallel даёт 50/50 dry/wet,
без него — 100% wet. Output находится до смешивания. При завершении bypass
истории сбрасываются. Алгоритм переключается дискретно со сбросом histories,
без morph и без обещания click-free switching.

Reduction=0 отключает новое gain reduction, но не переводит модель в Bypass:
histories продолжают обновляться. Normal пропускает встроенные аудиофильтры
окраски; они остаются активными только в Emulation.
Telemetry reduction снимается с применённого compression gain до makeup/output
и смешивания: положительный peak dB за последний блок. [Runtime](Runtime.md).

Исследовательский [DeviceCompressor VST3](DeviceCompressor.md) использует тот же
набор Normal-моделей, но владеет собственными host-параметрами. Output ручной,
динамического выходного Auto Gain нет.
