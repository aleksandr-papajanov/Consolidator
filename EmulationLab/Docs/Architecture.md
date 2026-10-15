# EmulationLab architecture

EmulationLab — offline pipeline исследования, отделённый от runtime основного
устройства. Граница использования и артефактов — [Rules](Rules.md).

```text
experiment manifest / conditions → reference и candidate VST3 host
→ probes / rendered audio / parameter readback → dataset
→ comparison / diagnostics / bounded fitting → принятые выводы
→ изменение общего Native algorithm отдельным production scope
```

| Scope | Роль |
| --- | --- |
| CLI и experiment configuration | Выбор plugins, conditions, overrides и операции |
| Host | Discovery, plugin lifecycle и buffers |
| Probes / analysis | Измерения и признаки для сравнения |
| Datasets / metadata | Значения, schema/signature, environment и readback |
| Optimization | Bounded поиск с checkpoints и согласованными conditions |

Стенд не владеет прикладным состоянием Consolidator. Candidate adapters находятся
в ModelPlugins.Vst3 и используют общий Native processor; собственные host controls
adapters не переносятся в архитектуру основного VST3. Raw audio, временные outputs
и принятые model findings имеют разные lifecycle и хранение.

## Аспекты

- [Компрессоры: случайные сигналы и история уровня](Aspects/CompressorSignalInvestigation.md)
- [Артефакты экспериментов](Aspects/ExperimentArtifacts.md)
- [VST3 discovery cache](Aspects/HostCaching.md)
- [Интерпретация измерений](Aspects/MeasurementMethods.md)
- [Consolidator model VST3 plugins](Aspects/ModelPluginsVst3.md)
- [Raw Audio](Aspects/RawAudio.md)
- [EmulationLab: ручные исследования DSP](Aspects/Usage.md)
