# PolishProcessor

[Исходник](../../Consolidator.Native/External/Dsp/PolishProcessor.cpp).

Вход → [Thick](PolishThickProcessor.md) → [Air](PolishAirProcessor.md).

Каждый добавляет enhancement к входящему сигналу; усреднения двух dry+wet веток нет.

Controls0…1 с ramps10 мс.

Activity/bypass stage принадлежит графу.

Tuning задаётся до Prepare, runtime histories принадлежат дочерним процессорам.
