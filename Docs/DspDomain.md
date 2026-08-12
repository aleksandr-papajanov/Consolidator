# DSP Domain

## Назначение

`Consolidator.Native/Source/Dsp` — независимый DSP-слой обработки аудио. Он не зависит от Max/Min и содержит цепочку устройств, обработчики аудио и runtime-состояние. Общие domain-типы находятся в `Source/Core/Domain`.

Главная цепочка собирается `DspChainBuilder`:

```text
Input Gain → Saturator → Compressor → Equalizer banks → Output Gain
```

`DspChain` хранит устройства и вызывает их `Process()` последовательно. `active` —
derived runtime state, публикуемый `ProcessingStateResolver`; неактивные устройства
пропускаются до вызова `Process()`. `IsNeutral()` остаётся отдельной оптимизацией
для математически нейтральных устройств.

## Устройства и состояние

Все аудиоустройства наследуются от `DspDevice` и имеют:

- `*RuntimeState` — локальная DSP-копия target values, производные величины и
  изменяемая audio-thread память;
- `RecalculateRuntime()` — пересчёт runtime после применения update batch.

Пользовательские `*State` принадлежат coordinator-owned `StateStore` и не
изменяются DSP. `ChainState` хранит отдельные input/output gain, top-level
devices, EQ banks, EQ filters и detector filters. DSP получает `ParameterUpdateBatch` через latest-value mailbox. Для
каждого `StatePath` mailbox хранит только последнее значение в атомарно
упакованном слоте; update содержит
монотонный `revision` для диагностики и порядка внутри batch. Audio thread
применяет batch перед `Process()`. Несхлопываемые события (например,
reset) должны идти отдельной event queue.

Параметры хранятся в `ParameterState<T>` из `Core/Domain/State`, но только внутри
`StateStore`. Числовой параметр содержит локальный `ParameterId`, текущее значение
и диапазон. Пользовательские boolean markers (`Bypass`, `Solo`, `Listen`, `Mute`)
хранятся в `StateMarker<bool>` и не являются DSP parameters.
Диапазоны и defaults задаются в `Core/Settings/DspDeviceSettings.h`; DSP
processor-классы не должны дублировать пользовательские параметры или их
диапазоны.

State-модель организована так:

```text
StateStore
├─ InstanceState
│  └─ InstanceAudibilityState
│     ├─ mute
│     └─ solo
└─ ChainState
   ├─ GainState
   ├─ SaturatorState
   │  └─ DetectorState
   ├─ CompressorState
   │  └─ DetectorState
   ├─ EqualizerState
   ├─ EqualizerBankState[]
   └─ GainState
```

Все DSP state создаются фабрикой `StateStore.Factory.cpp` из
`DspSettings`. Структуры из `DspStates.h` являются пассивными контейнерами
данных и не содержат routing или write-логики.

## Маршрутизация параметров

Параметры передаются как `StateEntry` с `StateField::DspParameter`. DSP markers
передаются с `StateField::DspMarker` и `StateMarkerId`; `Mute` и `Solo` остаются
отдельными state fields для instance state. `StatePath`
является единым адресом topology, DSP-параметров и DSP markers:

```cpp
StateEntry{
    StatePath{
        .field = StateField::DspParameter,
        .deviceId = DeviceId::Equalizer,
        .parameterId = ParameterId::Gain,
        .nodes = {RouteNodeId::Bank0, RouteNodeId::Filter3},
        .depth = 2},
    StateValue{6.0f}
};
```

`StatePath` имеет фиксированную глубину и не выделяет память. Каждый
composite-узел получает тот же `StateEntry` и передаёт его дальше:

```text
StateStore → RuntimeUpdateMailbox → DspChain → DspDevice → RuntimeState
```

`Solo` и `Bypass` являются authoritative processing markers в `StateStore`. При изменении
любого из них `ProcessingStateResolver` пересчитывает derived processing state.
В domain state они представлены `StateMarker<bool>`, а не `ParameterState<bool>`;
`StateMarkerId` отделяет markers от `ParameterId` и не делает их DSP runtime
параметром. То же правило действует для detector `Listen`.
Chain рассматривает весь Equalizer как одну стадию; banks являются peer-scope
внутри этой стадии. `EqualizerState` содержит markers всего EQ на depth 0,
а `EqualizerBankState` — markers конкретного банка на depth 1. Resolver публикует
полный derived `RuntimeResolution` как `RuntimeControlUpdate` в control mailbox.
`RuntimeProperty` разделяет `Active`, `Listen` и `OutputEnabled`, а composite key
`(target, property)` не позволяет им перетирать друг друга.
Processing state не входит в `StateStore` и не
сохраняется через state protocol. `DspChain::Process()` не вычисляет solo-правила.

`InstanceState::audibility.solo` имеет instance/group scope и не участвует в локальном
chain resolver. `InstanceState::audibility.mute` безусловно выключает instance.
`InstanceCoordinator` владеет `InstanceAudibilityResolver` и пересчитывает
`OutputEnabled` сразу для
всех live instances: при наличии output solo audible становится union source
instance и direct members его selected-bank group. Connected/transitive group
traversal для audibility не используется. `OutputEnabled` применяется на
instance output gate после `DspChain::Process()`; `OutputGain.bypass` остаётся
обычным локальным `Active` control.

Для chain границей становится самый downstream solo: upstream до него включительно
остаётся active, downstream устройства выключаются, а output gain остаётся
mandatory post stage. Для bank/filter peer scope при наличии solo active остаются
только solo-элементы; без solo действует `!bypass`.

Detector filters имеют отдельный peer-scope для Saturator и Compressor.
Их active дополнительно ограничивается состоянием parent device: detector нужен,
если parent active или включён Listen.
Их `RuntimeControlUpdate` paths проходят через `Detector` и не зависят от EQ-bank
solo. `Listen` является отдельным monitoring dimension и не является частью
`active` routing. `Listen` хранится только в `DetectorState`: это один marker
для всего detector Saturator и один для всего detector Compressor. Он доставляется
с `RuntimeProperty::Listen` через control mailbox. При Listen
полный detector output напрямую становится output устройства и обходит
основную обработку (gain reduction, saturation, wet/dry и output gain).
Detector envelope при этом продолжает рассчитываться обычным path.

`ResetDspCommand` clears a
device route's internal real-time memory through the realtime command SPSC queue;
composite devices recursively route EQ-bank, filter and detector-filter paths.
Reset events are not coalesced with parameter updates.

Для Saturator и Compressor detector route проходит через `RouteNodeId::Detector`, затем в их detector equalizer и соответствующий filter.

В batch сначала staging-ятся все target values, затем один раз вызывается
`CommitRuntimeUpdates()` у каждого устройства. Это гарантирует актуальный
`isNeutral` и производные коэффициенты без повторного пересчёта после каждого
параметра. Затем применяются `RuntimeControlUpdate` только для routing flags;
они не вызывают commit. После parameter/control snapshots исполняются ordered
realtime commands, и только затем начинается обработка аудиоблока. Это
block-start invariant. DSP не возвращает пользовательское состояние обратно в
coordinator.

## State boundary

`StateStore` implements all user-facing reads and writes. DSP devices expose
only `StageRuntimeUpdate(path, value)`, `CommitRuntimeUpdates()` and `Process()`.
They do not participate in state reads or coordinator command routing.

## Equalizer и detectors

`Equalizer` — общий composite-процессор для EQ banks и detector pre-filtering. Он владеет обычными filter-классами (`LowShelfFilter`, `BellFilter`, `HighShelfFilter`, `TiltFilter`, `GainFilter`).

Detector EQ не имеет bank. Bank — это только route segment обычного пользовательского equalizer.

`DetectorEnvelopeFollower` владеет `Equalizer`, затем выполняет rectification и attack/release envelope following. В Saturator используется по одному follower на активный канал.

Compressor напрямую владеет detector `Equalizer`; отдельная обёртка `CompressorSidechain` не используется.

## Compressor meters

- `CompressorMeterState` собирает linear attenuation за audio block: RMS
  attenuation и minimum attenuation. `Compressor::GetBlockTelemetry()`
  переводит их в positive reduction dB для snapshot.
- `RmsDetectorMeterState` хранит atomic RMS level в linear scale.

RMS detector измеряет уровень detector signal. Gain reduction принадлежит Compressor: она зависит от threshold, ratio, knee и attack/release smoothing.

## DSP telemetry

`Dsp/Telemetry/Telemetry.h` содержит `dsp::TelemetrySnapshot` и типизированные
точки `dsp::MeterPoint`. `Saturator` считает distortion до output gain и
wet/dry mix как отношение RMS нелинейного residual к RMS linear reference:
`distortionPercent = residualRms / max(linearRms, epsilon) * 100`. Это
normalized nonlinearity metric, а не THD: спектральный анализ гармоник не
выполняется.
`DspChain` только собирает chain levels и забирает processor telemetry через
cached processor pointers. Collection is enabled only for the instance selected
by the global `AnalysisView`; inactive instances do not perform meter
accumulation.

`MeterSmoother` сглаживает значения по времени с константой 150 ms и учитывает
реальную длительность audio block (`frameCount / sampleRate`). Level RMS
сглаживается в linear domain и конвертируется в dB после smoothing. Compressor
reduction RMS сначала вычисляется в linear attenuation и публикуется как
positive dB. UI-facing smoothed reduction затем сглаживается непосредственно
в positive dB с нейтральным начальным значением `0 dB`.
`PeakMeter` хранит block peak с мгновенной атакой, hold 75 ms и release 300 ms,
поэтому transient сохраняется даже если UI пропустил промежуточные latest
snapshots. Этот же persistent peak state используется для compressor reduction.

## Real-time правила

- В `Process()` нет аллокаций, mutex, логирования или I/O.
- Буферы и filter memory preallocated.
- UI/meter данные передаются lock-free latest snapshot-ами. Telemetry публикуется
  после каждого audio block без worker; Max читает только последний snapshot.
- FFT и offline analysis не выполняются в audio thread.

FFT input is accumulated in a preallocated analysis slot by the audio thread.
The completed window is published as a latest-value mailbox item to the global
`AnalysisService`; FFT calculation and spectrum publication happen on its one
background worker. The audio thread never waits for analysis and does not use
an analysis FIFO.

## Defaults

Единый источник user-facing defaults и limits — `Core/Settings/DspDeviceSettings.h`.

Локально в DSP допустимы только runtime identity values и математические/алгоритмические константы, не являющиеся пользовательскими настройками.
