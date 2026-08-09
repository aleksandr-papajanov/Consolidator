# DSP Domain

## Назначение

`Consolidator.Native/Source/Dsp` — независимый DSP-слой обработки аудио. Он не зависит от Max/Min и содержит цепочку устройств, обработчики аудио и runtime-состояние. Общие parameter-типы находятся в `Source/Core/Parameters`.

Главная цепочка собирается `DspChainBuilder`:

```text
Input Gain → Saturator → Compressor → Equalizer banks → Output Gain
```

`DspChain` хранит устройства и вызывает их `Process()` последовательно. Неактивные (`IsNeutral()`) устройства пропускаются.

## Устройства и состояние

Все аудиоустройства наследуются от `DspDevice` и имеют:

- `*State` — пользовательские параметры из `Source/Core/State`;
- `*RuntimeState` — производные DSP-величины и изменяемая audio-thread память;
- `RecalculateRuntime()` — пересчёт runtime после успешного изменения параметра.

Параметры хранятся в `DspParameter<T>` из `Core/Parameters`. Он содержит локальный `ParameterId`, текущее значение и диапазон. Диапазоны и defaults задаются в `Core/Settings/DspDeviceSettings.h`; processor-классы не должны дублировать их литералами.

## Маршрутизация параметров

Параметры передаются как `StateEntry` с `StateField::DspParameter` и точным `ParameterRoute`:

```cpp
StateEntry{
    StatePath{
        .field = StateField::DspParameter,
        .parameterRoute = ParameterRoute{
            DeviceId::Equalizer,
            ParameterId::Gain,
            RouteNodeId::Bank0,
            RouteNodeId::Filter3}},
    StateValue{6.0f}
};
```

`ParameterRoute` имеет фиксированную глубину и не выделяет память. Каждый composite-узел consumes свой segment и передаёт маршрут дальше:

```text
DspChain → DspDevice → Equalizer → Filter → DspParameter
```

Для Saturator и Compressor detector route проходит через `RouteNodeId::Detector`, затем в их detector equalizer и соответствующий filter.

После успешного изменения leaf parameter вызывается `RecalculateRuntime()` затронутого устройства. Это гарантирует актуальный `isNeutral` и производные коэффициенты.

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
