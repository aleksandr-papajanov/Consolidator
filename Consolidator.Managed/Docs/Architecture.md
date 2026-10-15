# Managed architecture

Managed разделяет reusable state mechanism, application Core, wire protocol и
NativeAOT boundary. Обязательное ownership описано в [Rules](Rules.md).

| Scope | Роль |
| --- | --- |
| `State/` | Generic registry/tree/values/history/observers без application concepts |
| `Core/State/` | Definitions, factory, concrete models, policies и observers |
| `Core/Commands/` | Typed commands, validation, target resolution и transactions |
| `Core/Services` | Shared/per-instance lifecycle, publication и persistence services |
| `Core/Topology/` | Derived indexes для state и routing |
| `Protocol/` | Codecs, typed dispatch, notifications и transport contracts |
| `Native/` | Exported ABI и marshaling без business logic |
| `Analysis/`, `Approximation/` | Worker analysis и control-path fitting |

```text
intent → protocol/typed command → target resolution under operation gate
→ prepared state transaction / dependent edits / peers / history
→ commit → observers / notifications / DSP publication
```

State definitions задают defaults/ranges/history policy, factory компонует
ownership/observers, а models связывают прикладные значения. Generic State не
знает устройств, topology и protocol. Operation gates сериализуют полные
прикладные действия; worker/audio input boundary отделён от control logic.
Published parameters и runtime telemetry — разные потоки данных.

## Аспекты

State storage, propagation, topology, audibility и history описаны раздельно.
Protocol определяет wire contract; persistence — committed capture/restore;
StatePipeline — связь с DSP; устройство описывает только собственные значения
и поведение, используя эти общие механизмы.

- [Equalizer bank approximation](Aspects/Approximation.md)
- [Audibility projection](Aspects/Audibility.md)
- [Bank Index Contract](Aspects/BankIndexContract.md)
- [Managed Coordination](Aspects/Coordinator.md)
- [Gain Staging state and DSP projection](Aspects/GainStaging.md)
- [Shaper state and DSP projection](Aspects/Shaper.md)
- [Compression state and DSP projection](Aspects/Compression.md)
- [Instance controls and registry status](Aspects/InstanceControls.md)
- [Managed Protocol](Aspects/ManagedProtocol.md)
- [Peer propagation and constraints](Aspects/PeerPropagation.md)
- [Persistence](Aspects/Persistence.md)
- [Processor Activity](Aspects/ProcessorActivity.md)
- [State reset commands](Aspects/ResetCommands.md)
- [Runtime Metrics Diagnostics](Aspects/RuntimeMetrics.md)
- [Spectrum capture and worker analysis](Aspects/SpectrumAnalysis.md)
- [Spectrum frame protocol](Aspects/SpectrumProtocol.md)
- [Managed history and transactions](Aspects/StateHistory.md)
- [State Infrastructure](Aspects/StateInfrastructure.md)
- [Managed DSP state projection](Aspects/StatePipeline.md)
- [State values and observer order](Aspects/StateValues.md)
- [Target observation protocol](Aspects/TargetObservation.md)
- [DSP Telemetry](Aspects/Telemetry.md)
- [Topology indexes and observers](Aspects/Topology.md)
