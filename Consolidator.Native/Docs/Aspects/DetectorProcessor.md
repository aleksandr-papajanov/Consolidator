# DetectorProcessor

[Исходник](../../External/Dsp/DetectorProcessor.cpp).

Копирует main audio в preallocated private buffers, применяет LowShelf/Bell каскады и публикует read-only detectorSignal.

EQ детектора не слышен в main audio.

Input capture после Width; Saturator capture на входе; Compressor отдельно до Compression и после Compression/makeup перед Transient.

У каждого capture собственная история фильтров.
