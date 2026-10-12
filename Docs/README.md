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
| [DSP](Dsp/README.md) | Правила, каждый процессор, runtime, state и ручные исследования |
| [CLA-76 findings](../Consolidator.Native/External/Dsp/Processors/Cla76/Findings.md) | Краткие полезные выводы завершённого исследования |
| [CLA-3A](../Consolidator.Native/External/Dsp/Processors/Cla3a/README.md) | Общая Native-модель с референсными Gain/Peak Reduction |
| [CLA-3A findings](../Consolidator.Native/External/Dsp/Processors/Cla3a/Findings.md) | Краткие выводы исследований и ограничения упрощённой модели |

Новый самостоятельный scope сразу добавлять в этот индекс или индекс DSP.
Документация соответствует текущему коду; историю гипотез хранить кратко рядом с процессором.

Finished simplified model: [SSLChannel compressor](../Consolidator.Native/External/Dsp/Processors/SslChannel/README.md), Ratio/Release/Attack/Threshold.

[SSLChannel findings](../Consolidator.Native/External/Dsp/Processors/SslChannel/Findings.md): initial measurements and remaining limits.

## Гайд для будущей эмуляции

- [Skill: consolidator-dsp-emulation](../.agents/skills/consolidator-dsp-emulation/SKILL.md) — гибкий проектный гайд по эмуляции и упрощению DSP.
- [Методы](../.agents/skills/consolidator-dsp-emulation/references/methods.md) — выбор измерений, подбор функций и оптимизация.
- [Примеры устройств](../.agents/skills/consolidator-dsp-emulation/references/device-lessons.md) — выводы из CLA, SSLChannel, Transient Shaper, Air и Thick.
