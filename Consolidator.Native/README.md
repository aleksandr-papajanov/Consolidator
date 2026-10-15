# Consolidator.Native

Выполняет обработку звука. Основной channel strip находится в Audio; общий ABI и
транспорт к Managed — в Transport. Исследовательские VST3 также вызывают актуальные
Native processors из Audio; прежний External-код не является частью проекта.

[Решение](../README.md)

## Документация

- [Правила](Docs/Rules.md)
- [Форматирование](Docs/Formatting.md)
- [Архитектура](Docs/Architecture.md)

## Подробные аспекты

- [PolishAirProcessor](Docs/Aspects/Air.md)
- [Air / Revival Shimmer: полезные выводы](Docs/Aspects/AirFindings.md)
- [Cla2aProcessor](Docs/Aspects/Cla2a.md)
- [CLA-2A identification notes](Docs/Aspects/Cla2aFindings.md)
- [Cla3aProcessor](Docs/Aspects/Cla3a.md)
- [CLA-3A findings](Docs/Aspects/Cla3aFindings.md)
- [Cla76Processor](Docs/Aspects/Cla76.md)
- [CLA-76 useful research findings](Docs/Aspects/Cla76Findings.md)
- [Управление четырьмя компрессорами](Docs/Aspects/CompressorControlModes.md)
- [CompressorDevice](Docs/Aspects/CompressorProcessor.md)
- [DetectorEqualizer](Docs/Aspects/DetectorProcessor.md)
- [DeviceCompressorProcessor](Docs/Aspects/DeviceCompressor.md)
- [Universal compressor notes](Docs/Aspects/DeviceCompressorFindings.md)
- [EqualizerDevice](Docs/Aspects/EqualizerProcessor.md)
- [Filter definitions](Docs/Aspects/FilterCatalog.md)
- [DSP implementation structure](Docs/Aspects/Implementation.md)
- [InputDevice](Docs/Aspects/InputProcessor.md)
- [LevelerProcessor](Docs/Aspects/LevelerProcessor.md)
- [LimiterProcessor](Docs/Aspects/LimiterProcessor.md)
- [OutputDevice](Docs/Aspects/OutputProcessor.md)
- [PolishDevice](Docs/Aspects/PolishProcessor.md)
- [Shared processor models and configuration](Docs/Aspects/ProcessorModels.md)
- [Компенсация громкости по музыкальным источникам](Docs/Aspects/ProgramMakeup.md)
- [DSP Runtime Foundation](Docs/Aspects/Runtime.md)
- [SaturationProcessor](Docs/Aspects/SaturationProcessor.md)
- [SaturatorDevice](Docs/Aspects/SaturatorProcessor.md)
- [SslChannelProcessor](Docs/Aspects/SslChannel.md)
- [SSLChannel findings](Docs/Aspects/SslChannelFindings.md)
- [PolishThickProcessor](Docs/Aspects/Thick.md)
- [Thick: полезные выводы](Docs/Aspects/ThickFindings.md)
- [TransientShaperProcessor — общий Native / VST3 DSP](Docs/Aspects/TransientShaper.md)
- [Transient Shaper: полезные выводы](Docs/Aspects/TransientShaperFindings.md)
