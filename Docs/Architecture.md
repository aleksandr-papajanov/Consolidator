# Архитектура решения

Решение разделено на управляющий слой, обработку аудио, host/presentation и
отдельный исследовательский стенд. Правила изменения границ — [Rules](Rules.md).

| Scope | Техническая роль |
| --- | --- |
| Managed | Авторитетное state tree, команды, topology/peers, history/persistence, worker analysis и DSP projection |
| Native | Общие DSP algorithms/runtime и низкоуровневый Managed transport |
| VST3 | Host lifecycle/audio/state callbacks, protocol clients и JUCE presentation |
| EmulationLab | Offline host, experiment conditions, probes, datasets и fitting |

```text
VST3 intent → Managed command → state transaction / peers / observers
                                  ├→ DSP snapshot → Native runtime → audio
                                  └→ notification → ViewModel/binding → JUCE control

main / sidechain → bounded audio capture → Managed worker → FFT presentation
```

Managed state и Native runtime history различаются по ownership: первое хранит
прикладные значения, второе — состояние исполнения алгоритма. VST3 preview не
заменяет committed values. NativeAOT lifecycle не зависит от наличия editor.
EmulationLab не входит в runtime основного устройства; исследовательские model
adapters вызывают те же Native algorithms, но имеют собственный host-контракт.

## Архитектуры проектов

- [Managed](../Consolidator.Managed/Docs/Architecture.md): state, commands и observers.
- [Native](../Consolidator.Native/Docs/Architecture.md): DSP runtime и transport.
- [VST3](../Consolidator.Vst3/Docs/Architecture.md): host и presentation.
- [EmulationLab](../EmulationLab/Docs/Architecture.md): исследовательский pipeline.

## Общие аспекты

Подробности ниже задают межпроектные контракты; правила отдельного устройства
хранятся у его владельца, без второй копии ABI или алгоритма.

- [Realtime audio input ABI](Aspects/AudioInput.md)
- [DSP snapshot exchange](Aspects/DspStateExchange.md)
- [Managed runtime and instance lifecycle](Aspects/InstanceLifecycle.md)
- [C ABI and borrowed data](Aspects/InteropAbi.md)
- [Interop и lifetime](Aspects/InteropRules.md)
- [Realtime boundary](Aspects/RealtimeBoundary.md)
- [Testing](Aspects/Testing.md)
