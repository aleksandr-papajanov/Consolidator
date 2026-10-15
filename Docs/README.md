# Documentation Index

Документы разделены по scope. [DSP: алгоритмы и EmulationLab](Dsp/README.md).

Код и документация законченных DSP-моделей:
[Processors](../Consolidator.Native/External/Dsp/Processors/README.md).

Завершённая упрощённая модель: [CLA-2A](../Consolidator.Native/External/Dsp/Processors/Cla2a/README.md).
[Наблюдения CLA-2A](../Consolidator.Native/External/Dsp/Processors/Cla2a/Findings.md).

| Документ | Scope |
| --- | --- |
| [BankIndexContract.md](BankIndexContract.md) | BankIndexContract |
| [CSharpFormatting.md](CSharpFormatting.md) | CSharpFormatting |
| [Coordinator.md](Coordinator.md) | Coordinator |
| [CppFormatting.md](CppFormatting.md) | CppFormatting |
| [Formatting.md](Formatting.md) | Formatting |
| [JavaScriptFormatting.md](JavaScriptFormatting.md) | JavaScriptFormatting |
| [ManagedNativeCommunication.md](ManagedNativeCommunication.md) | ManagedNativeCommunication |
| [ManagedProtocol.md](ManagedProtocol.md) | ManagedProtocol |
| [MaxV8RuntimeDiagnostics.md](MaxV8RuntimeDiagnostics.md) | MaxV8RuntimeDiagnostics |
| [Persistence.md](Persistence.md) | Persistence |
| [Rules.md](Rules.md) | Rules |
| [RuntimeMetrics.md](RuntimeMetrics.md) | RuntimeMetrics |
| [StateHistory.md](StateHistory.md) | StateHistory |
| [StateInfrastructure.md](StateInfrastructure.md) | StateInfrastructure |
| [Testing.md](Testing.md) | Testing |
| [UiPerformance.md](UiPerformance.md) | UiPerformance |
| [V8Migration.md](V8Migration.md) | V8Migration |
| [Vst3Migration.md](Vst3Migration.md) | Windows JUCE VST3 migration plan, host foundation and current limitations |
| [Vst3Editor.md](Vst3Editor.md) | Existing UI behavior port, gestures, Bank Manager and graphical analyzer |
| [Vst3Controls.md](Vst3Controls.md) | JUCE dial controls, precision, discrete controls and Input Leveler runtime presentation |
| [Vst3Theme.md](Vst3Theme.md) | Shared JUCE LookAndFeel, control metrics and Bank Manager/analyzer buttons |
| [Vst3AnalyzerRendering.md](Vst3AnalyzerRendering.md) | Analyzer path caches, spectrum peak preservation, response resolution and plot clipping |
| [Vst3ChannelStrip.md](Vst3ChannelStrip.md) | Fixed canvas scaling, simultaneous processor columns and graphical-only filter editing |
| [Vst3SidechainReference.md](Vst3SidechainReference.md) | Stereo Sidechain through Managed capture/FFT, shared notification and overlay visibility |
| [Vst3Architecture.md](Vst3Architecture.md) | Feature-first VST3 organization, Shared/Host boundaries, namespaces, client and binding ownership |
| [Vst3FeatureRefactor.md](Vst3FeatureRefactor.md) | Tracks/ChannelStrip/Equalizer boundaries, shared target context and implementation sequence |
| [Vst3ParityReview.md](Vst3ParityReview.md) | Old/new UI behavior mapping, omissions corrected and reasons for platform/architecture differences |
| [DSP](Dsp/README.md) | Правила, каждый процессор, runtime, state и ручные исследования |
| [CLA-76 findings](../Consolidator.Native/External/Dsp/Processors/Cla76/Findings.md) | Краткие полезные выводы завершённого исследования |
| [CLA-3A](../Consolidator.Native/External/Dsp/Processors/Cla3a/README.md) | Общая Native-модель с референсными Gain/Peak Reduction |
| [CLA-3A findings](../Consolidator.Native/External/Dsp/Processors/Cla3a/Findings.md) | Краткие выводы исследований и ограничения упрощённой модели |

Новый самостоятельный scope сразу добавлять в этот индекс или индекс DSP.
Документация соответствует текущему коду; историю гипотез хранить кратко рядом с процессором.

Shared Native model: [SSLChannel compressor](../Consolidator.Native/External/Dsp/Processors/SslChannel/README.md), reference controls retained in Emulation, selected through the universal compressor in Normal.

[SSLChannel findings](../Consolidator.Native/External/Dsp/Processors/SslChannel/Findings.md): initial measurements and remaining limits.

## Гайд для будущей эмуляции

- [Skill: consolidator-dsp-emulation](../.agents/skills/consolidator-dsp-emulation/SKILL.md) — гибкий проектный гайд по эмуляции и упрощению DSP.
- [Методы](../.agents/skills/consolidator-dsp-emulation/references/methods.md) — выбор измерений, подбор функций и оптимизация.
- [Примеры устройств](../.agents/skills/consolidator-dsp-emulation/references/device-lessons.md) — выводы из CLA, SSLChannel, Transient Shaper, Air и Thick.

[Режимы управления компрессорами](Dsp/CompressorControlModes.md): Normal Reduction/Output и сохранённый Native Emulation.

[Универсальный компрессор](../Consolidator.Native/External/Dsp/Processors/DeviceCompressor/README.md),
[наблюдения](../Consolidator.Native/External/Dsp/Processors/DeviceCompressor/Findings.md): четыре алгоритма в одном Native/VST3 устройстве.

[Компенсация по музыкальным источникам](Dsp/ProgramMakeup.md): метод подбора,
аналитические законы Normal и оставшиеся различия между алгоритмами.

[Исследование случайных сигналов](Dsp/CompressorSignalInvestigation.md): переходы
уровня, импульсы, тишина и поведение компрессоров при 44.1/96 kHz.

[Адаптивный автогейн](Dsp/AdaptiveCompressorGain.md): общий измеритель dry/wet,
галочка Auto Gain и видимое управление Output в универсальном компрессоре.
