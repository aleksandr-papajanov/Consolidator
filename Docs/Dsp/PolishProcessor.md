# PolishProcessor

[Исходник](../../Consolidator.Native/External/Dsp/PolishProcessor.cpp).

Вход → [Thick](../../Consolidator.Native/External/Dsp/Processors/Thick/README.md) → [Air](../../Consolidator.Native/External/Dsp/Processors/Air/README.md).

Каждый добавляет enhancement к входящему сигналу; усреднения двух dry+wet веток нет.

Controls0…1 с ramps10 мс.

Activity/bypass stage принадлежит графу.

Immutable configuration принадлежит Native headers; research overrides и
API изменения коэффициентов удалены. Runtime histories принадлежат дочерним процессорам.
