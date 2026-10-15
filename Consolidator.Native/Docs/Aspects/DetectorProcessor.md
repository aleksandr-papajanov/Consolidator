# DetectorEqualizer

[Исходник](../../Audio/Devices/DetectorEqualizer.cpp).

Копирует текущий main в подготовленные private buffers и применяет LowShelf/Bell.
Возвращает read-only DetectorBuffer. Main audio не меняется. Заимствованные указатели
валидны до следующего Capture того же экземпляра; число samples задаёт main buffer.

Независимые экземпляры принадлежат Input, Saturation, Compression и Output.
Их точки capture и source/state contracts определены в [Runtime](Runtime.md).
У Shaper собственный HP20 detector внутри алгоритма.
