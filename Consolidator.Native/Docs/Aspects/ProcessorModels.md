# Processor models and configuration

Основные model sources находятся в `Audio/Algorithms/<Model>`, namespace
`consolidator::audio::algorithms`. Runtime/Devices интегрируют их в channel strip;
звуковые constants и histories принадлежат модели. [Общая структура](Runtime.md).

| Model | Контракт | Исследования |
| --- | --- | --- |
| Cla76 | [FET](Cla76.md) | [Findings](Cla76Findings.md) |
| Cla3a | [Optical](Cla3a.md) | [Findings](Cla3aFindings.md) |
| Cla2a | [Optical](Cla2a.md) | [Findings](Cla2aFindings.md) |
| SslChannel | [SSL](SslChannel.md) | [Findings](SslChannelFindings.md) |
| TransientShaper | [Signed masks и постоянные clamp](TransientShaper.md) | [Findings](TransientShaperFindings.md) |
| Air | [Contour и upper harmonics](Air.md) | [Findings](AirFindings.md) |
| Thick | [Bass body и harmonic enhancement](Thick.md) | [Findings](ThickFindings.md) |

Headers содержат controls, immutable inline static constexpr Config и per-instance
runtime state. Нет runtime setters для constants и research overrides. Prepare,
control updates, detector/gain laws и processing выделены в отдельные методы.
Общие ramps/filters/math принадлежат Primitives, параметры channel strip — Managed.

[CompressorDevice](CompressorProcessor.md) выбирает одну из четырёх Normal-моделей,
подаёт собственный detector EQ и возвращает applied reduction в telemetry.
[Control modes](CompressorControlModes.md) сохраняет контракт Normal/Emulation;
основная цепочка использует Normal. Профили controls принадлежат Managed.
Shaper — отдельное предыдущее устройство; Polish составляет Thick -> Air.

Исследовательские adapters компилируют те же модели из `Audio`, включая
[DeviceCompressor](DeviceCompressor.md). Их собственные host-параметры и state не
меняют владельца алгоритмов. [DSP rules](../Rules.md).
