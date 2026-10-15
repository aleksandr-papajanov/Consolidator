# Native architecture

Native содержит общий DSP и transport к Managed. Граница прикладного состояния
определена в [Rules](Rules.md); общий ABI — в [архитектуре решения](../../Docs/Architecture.md).

| Scope | Роль |
| --- | --- |
| `External/Dsp` | Graph/runtime, snapshot consumption, общие primitives, ramps и filters |
| `External/Dsp/Processors` | Общие алгоритмы устройств и их audio-owned states |
| `External/Transport` | ManagedBridge, ABI types и копирование borrowed данных |

Host передаёт подготовленные buffers и контекст sample rate/block capacity.
Managed публикует параметры через фиксированный snapshot; Native runtime
потребляет его и сохраняет собственные envelopes, histories и smoothing.
Prepare/reset/process относятся к lifecycle исполнения, а не к UI state.
Host-specific APIs остаются за пределами DSP.

Composite processors связывают primitives и device algorithms в фиксированный
graph. Основной VST3 и исследовательские adapters компилируют одни Native
исходники. Алгоритм не копируется в host или стенд. Телеметрия возвращает
ограниченную runtime-проекцию отдельно от параметров.

## Аспекты

Runtime и implementation описывают композицию и исполнение; отдельные device
documents — конкретные algorithms/control laws. Файлы Findings содержат
наблюдения исследования соответствующей модели, а не второй контракт алгоритма.

- [Auto Gain и видимый Output](Aspects/AdaptiveCompressorGain.md)
- [PolishAirProcessor](Aspects/Air.md)
- [Air / Revival Shimmer: полезные выводы](Aspects/AirFindings.md)
- [Cla2aProcessor](Aspects/Cla2a.md)
- [CLA-2A identification notes](Aspects/Cla2aFindings.md)
- [Cla3aProcessor](Aspects/Cla3a.md)
- [CLA-3A findings](Aspects/Cla3aFindings.md)
- [Cla76Processor](Aspects/Cla76.md)
- [CLA-76 useful research findings](Aspects/Cla76Findings.md)
- [Adaptive Compression](Aspects/CompressionProcessor.md)
- [Управление четырьмя компрессорами](Aspects/CompressorControlModes.md)
- [CompressorProcessor](Aspects/CompressorProcessor.md)
- [DetectorProcessor](Aspects/DetectorProcessor.md)
- [DeviceCompressorProcessor](Aspects/DeviceCompressor.md)
- [Universal compressor notes](Aspects/DeviceCompressorFindings.md)
- [EqualizerProcessor](Aspects/EqualizerProcessor.md)
- [Filter definitions](Aspects/FilterCatalog.md)
- [DSP implementation structure](Aspects/Implementation.md)
- [InputProcessor](Aspects/InputProcessor.md)
- [LevelerProcessor](Aspects/LevelerProcessor.md)
- [LimiterProcessor](Aspects/LimiterProcessor.md)
- [OutputProcessor](Aspects/OutputProcessor.md)
- [PolishProcessor](Aspects/PolishProcessor.md)
- [Shared processor models and configuration](Aspects/ProcessorModels.md)
- [Компенсация громкости по музыкальным источникам](Aspects/ProgramMakeup.md)
- [DSP Runtime Foundation](Aspects/Runtime.md)
- [SaturationProcessor](Aspects/SaturationProcessor.md)
- [SaturatorProcessor](Aspects/SaturatorProcessor.md)
- [SslChannelProcessor](Aspects/SslChannel.md)
- [SSLChannel findings](Aspects/SslChannelFindings.md)
- [PolishThickProcessor](Aspects/Thick.md)
- [Thick: полезные выводы](Aspects/ThickFindings.md)
- [TransientShaperProcessor — общий Native / VST3 DSP](Aspects/TransientShaper.md)
- [Transient Shaper: полезные выводы](Aspects/TransientShaperFindings.md)
