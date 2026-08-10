# DSP Domain

## Назначение

`Consolidator.Native/Source/Dsp` — независимый DSP-слой обработки аудио. Он не зависит от Max/Min и содержит цепочку устройств, обработчики аудио и runtime-состояние. Общие domain-типы находятся в `Source/Core/Domain`.

Главная цепочка собирается `DspChainBuilder`:

```text
Input Gain → Saturator → Compressor → Equalizer banks → Output Gain
```

`DspChain` хранит устройства и вызывает их `Process()` последовательно. Неактивные (`IsNeutral()`) устройства пропускаются.

## Устройства и состояние

Все аудиоустройства наследуются от `DspDevice` и имеют:

- `*RuntimeState` — локальная DSP-копия target values, производные величины и
  изменяемая audio-thread память;
- `RecalculateRuntime()` — пересчёт runtime после применения update batch.

Пользовательские `*State` принадлежат coordinator-owned `StateStore` и не
изменяются DSP. `ChainState` хранит отдельные input/output gain, top-level
devices, EQ banks, EQ filters и detector filters. DSP получает `DspStateBatch` через latest-value mailbox. Для
каждого `StatePath` mailbox хранит только последнее значение в атомарно
упакованном слоте; update содержит
монотонный `revision` для диагностики и порядка внутри batch. Audio thread
применяет batch перед `Process()`. Несхлопываемые события (например,
reset) должны идти отдельной event queue.

Параметры хранятся в `ParameterState<T>` из `Core/Domain/State`, но только внутри
`StateStore`. Числовой параметр содержит локальный `ParameterId`, текущее значение
и диапазон; boolean-параметр содержит только `ParameterId` и значение.
Диапазоны и defaults задаются в `Core/Settings/DspDeviceSettings.h`; DSP
processor-классы не должны дублировать пользовательские параметры или их
диапазоны.

State-модель организована так:

```text
StateStore
├─ InstanceState
└─ ChainState
   ├─ GainState
   ├─ SaturatorState
   │  └─ DetectorState
   ├─ CompressorState
   │  └─ DetectorState
   ├─ EqualizerBankState[]
   └─ GainState
```

Все DSP state создаются фабрикой `StateStore.Factory.cpp` из
`DspSettings`. Структуры из `DspStates.h` являются пассивными контейнерами
данных и не содержат routing или write-логики.

## Маршрутизация параметров

Параметры передаются как `StateEntry` с `StateField::DspParameter`. `StatePath`
является единым адресом topology и DSP-параметров:

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
StateStore → DspUpdateMailbox → DspChain → DspDevice → RuntimeState
```

Для Saturator и Compressor detector route проходит через `RouteNodeId::Detector`, затем в их detector equalizer и соответствующий filter.

В batch сначала staging-ятся все target values, затем один раз вызывается
`CommitRuntimeUpdates()` у каждого устройства. Это гарантирует актуальный
`isNeutral` и производные коэффициенты без повторного пересчёта после каждого
параметра. DSP не возвращает пользовательское состояние обратно в coordinator.

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

- `CompressorMeterState` хранит atomic snapshot gain reduction для UI.
- `RmsDetectorMeterState` хранит atomic RMS level в linear scale.

RMS detector измеряет уровень detector signal. Gain reduction принадлежит Compressor: она зависит от threshold, ratio, knee и attack/release smoothing.

## Real-time правила

- В `Process()` нет аллокаций, mutex, логирования или I/O.
- Буферы и filter memory preallocated.
- UI/meter данные передаются atomic snapshot-ами.
- FFT и offline analysis не выполняются в audio thread.

## Defaults

Единый источник user-facing defaults и limits — `Core/Settings/DspDeviceSettings.h`.

Локально в DSP допустимы только runtime identity values и математические/алгоритмические константы, не являющиеся пользовательскими настройками.
