# Native architecture

Native содержит общий DSP и transport к Managed. Прикладное состояние принадлежит
Managed; audio-owned histories принадлежат Native.

| Scope | Роль |
| --- | --- |
| `Audio/Core` | Заимствованные buffers, processing context и физические filter controls |
| `Audio/Primitives` | Общие обработки, ramps, filters, их cascades и измерители |
| `Audio/Algorithms` | Самостоятельные звуковые algorithms с собственными histories |
| `Audio/Devices` | Полные секции channel strip, detector EQ и transitions |
| `Audio/Chain` | Фиксированная ConsolidatorChain |
| `Audio/Runtime` | Snapshot binding, запуск цепочки и telemetry publication |
| `Transport` | ManagedBridge, shared ABI, snapshot exchange и borrowed-data boundary |

```text
Managed state -> DspSnapshot -> AudioRuntime / SnapshotBinding
    -> typed device parameters -> ConsolidatorChain -> main audio
Audio-owned measurements -> bounded telemetry -> Managed -> presentation
```

Подробные ответственность, signal order, lifecycle, namespaces и realtime boundaries:
[Runtime](Aspects/Runtime.md). Организация реализации: [Implementation](Aspects/Implementation.md).

Исследовательские `ModelPlugins.Vst3` adapters компилируют актуальные модели из
`Audio`; их host-параметры и state остаются самостоятельными от основного VST3.
Прежний `External` код не входит в проект, сборки или активные data paths.

Алгоритмические аспекты перечислены в [README](../README.md). Их control laws и
research findings сохраняют самостоятельные контракты. Исторические измерения не
подтверждают новую host integration.
