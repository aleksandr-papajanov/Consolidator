# OutputProcessor

[Исходник](../../Consolidator.Native/External/Dsp/OutputProcessor.cpp).

Ручной Output Level через DspGainRamp → [Limiter](LimiterProcessor.md), пока он включён или восстанавливает GR.

Level/Target сглаживаются10 мс; RequiresProcessing учитывает обе стадии.

После Output общий граф измеряет linked RMS; meter не корректирует gain.
