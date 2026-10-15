# VST3 architecture

VST3 разделён на host adapter и disposable presentation. Managed остаётся
владельцем state/команд, Native — DSP. Обязательная граница — [Rules](Rules.md).

| Scope | Роль |
| --- | --- |
| `Host/Plugin` | AudioProcessor/editor, runtime registration, audio и host state callbacks |
| `Host/Editor` | Композиция features, focus/activation, event routing и размещение |
| `Features/Tracks` | Bank Manager, reusable track/bank/group navigation, selection presentation и intents действий |
| `Features/ChannelStrip` | Processor control composition и bindings |
| `Features/Equalizer` | Graph, spectrum presentation, sources, overlays, analyzer client и EQ actions; размещает навигацию Tracks |
| `Shared` | Typed clients/protocol, ViewModels, generic controls, gesture transport и theme |

```text
Managed snapshot/notification → client accepted projection → ViewModel
→ binding → control
control gesture/action → typed intent → client command → Managed
```

Clients не зависят от widgets; ViewModels не вызывают JUCE controls; bindings
связывают accepted updates с intents. Controls владеют pointer/paint state и
временным preview. Features владеют внутренней композицией, host размещает их
и маршрутизирует события. Эти обязанности не меняются от экранного положения.

Processor живёт независимо от editor. UI и clients работают на message thread;
audio path использует подготовленные buffers, общий Native graph и Managed
capture boundary. Save/load передаёт Managed snapshot без второго UI state tree.
Подробности composition/lifetime и протокольного staging вынесены отдельно.

## Аспекты

- [VST3 analyzer rendering](Aspects/AnalyzerRendering.md)
- [VST3 build и publish](Aspects/Build.md)
- [Fixed VST3 channel strip](Aspects/ChannelStrip.md)
- [JUCE controls](Aspects/Controls.md)
- [Equalizer layout](Aspects/EqualizerLayout.md)
- [VST3 frontend architecture](Aspects/FrontendComposition.md)
- [User gestures and acknowledgements](Aspects/Gestures.md)
- [Graph editing and response preview](Aspects/GraphEditing.md)
- [Windows VST3 host](Aspects/Host.md)
- [Runtime Leveler overlay](Aspects/RuntimeOverlay.md)
- [VST3 sidechain reference spectrum](Aspects/SidechainReference.md)
- [Target snapshot assembly and stable presentation](Aspects/SnapshotAssembly.md)
- [VST3 JUCE theme](Aspects/Theme.md)
- [Track and bank interaction](Aspects/TrackInteraction.md)
- [VST3 lifecycle и диагностика](Aspects/Validation.md)
- [Viewer activation and target transitions](Aspects/ViewerLifecycle.md)
