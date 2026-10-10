# EqualizerProcessor

[Исходник](../../Consolidator.Native/External/Dsp/EqualizerProcessor.cpp).

Последовательный каскад 7 банков по 7 фильтров: Gain, Tilt, LowShelf, HighShelf и три Bell.

Учитываются active/bypass устройства и active банка.

Transposed direct form II имеет отдельные истории каналов.

При изменениях old/new outputs кроссфейдятся за 10 мс; сырые коэффициенты не интерполируются.

После перехода работает только текущий путь.

Формулы и ограничения находятся в [FilterCatalog](FilterCatalog.md).
