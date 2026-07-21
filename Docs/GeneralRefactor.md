# Consolidator: целевая архитектура после рефакторинга

## 1. Назначение документа

Этот документ описывает целевую архитектуру Consolidator после рефакторинга: кто владеет состоянием, как компоненты обмениваются данными, где проходит граница между runtime-состоянием, потоковыми данными, DSP, UI и persistence, а также в каком порядке переводить существующий проект на новую модель.

Главная цель рефакторинга — перейти от набора Max-компонентов, которые свободно обмениваются сообщениями и частично дублируют состояние друг друга, к централизованной, типизированной и предсказуемой системе.

---

# 2. Общая модель

В центре каждого инстанса устройства находится `DeviceHost`.

Все обычные компоненты:

- отправляют команды только в `DeviceHost`;
- получают подтверждённые уведомления и snapshots только от `DeviceHost`;
- не знают о существовании друг друга;
- не владеют официальным глобальным состоянием;
- не читают и не изменяют общий `Dictionary` напрямую.

Физическая Max-шина может остаться, но её роль меняется. Она становится только транспортом между компонентами и Host, а не свободной сетью общения.

```text
                         UI Controller
                               │
                               │ commands / events
                               ▼
Analyzer ◄────────────────► DeviceHost ◄────────────────► Approximator
                               │
                               │ state snapshots
                               ▼
                          DSP Engine
```

Для высокочастотных данных разрешены отдельные специализированные каналы:

```text
Analyzer ═══════ AnalysisStream ═══════► Approximator
```

Такой stream:

- создаётся для конкретной операции;
- имеет одного producer и одного consumer;
- не является общей шиной команд;
- не превращает Host в прокси каждого frame.

---

# 3. Главные архитектурные правила

## 3.1. Единственный источник истины

Официальное состояние устройства хранится только в типизированных stores внутри `DeviceHost`.

```cpp
struct DeviceState {
    EqState eq;
    CompressorState compressor;
    SaturatorState saturator;
    AnalyzerState analyzer;
    ApproximatorState approximator;
    GlobalState global;
};
```

UI, DSP, Analyzer и Approximator могут иметь локальные кеши, рабочие копии или immutable snapshots, но они не считаются источником истины.

Актуальным считается только состояние, которое:

1. принято Host;
2. провалидировано;
3. атомарно применено;
4. получило новую revision;
5. опубликовано как подтверждённое изменение.

---

## 3.2. Компоненты не знают друг о друге

Обычная коммуникация между компонентами запрещена.

Нежелательная модель:

```text
Analyzer → Approximator
Approximator → DspProcessor
UI → Analyzer
EqManager → DSP
```

Целевая модель:

```text
Component → DeviceHost → Component
```

Пример:

1. Approximator завершает fit.
2. Он отправляет Host typed result.
3. Host проверяет session ID, валидность и соответствие definitions.
4. Host обновляет `ApproximatorStore`.
5. При необходимости Host коммитит результат в `EqStore`.
6. DSP, UI и Painter независимо получают официальное обновление.

Approximator не должен напрямую устанавливать параметры DSP или двигать UI-контролы.

---

## 3.3. Команды и уведомления разделены

Команда означает намерение:

> Компонент просит Host изменить состояние или запустить действие.

Примеры:

```text
eq.set_parameter
eq.select_bank
fit.start
fit.cancel
analysis.capture
preset.load
```

Уведомление означает подтверждённый факт:

> Изменение уже принято, провалидировано и зафиксировано либо операция перешла в новое состояние.

Примеры:

```text
store.updated
operation.started
operation.completed
operation.failed
host.initialized
```

Исходная команда не является подтверждением.

Например, UI может запросить:

```text
eq.set_frequency 50000
```

Host ограничит значение допустимым диапазоном:

```text
frequency = 20000
```

Именно подтверждённое значение попадёт в store, DSP и UI.

---

## 3.4. Store и stream решают разные задачи

Store отвечает на вопрос:

> Какое устойчивое состояние сейчас актуально?

Stream отвечает на вопрос:

> Какие данные поступают прямо сейчас?

В stores допустимо хранить:

- параметры фильтров;
- активный банк;
- настройки компрессора и сатуратора;
- статус анализа;
- progress операции;
- session ID;
- последнюю агрегированную кривую;
- последний candidate;
- итоговый результат fit;
- ошибку операции.

Через stores не следует передавать:

- каждый audio block;
- каждый FFT frame;
- каждую итерацию оптимизатора;
- каждое промежуточное измерение;
- другие высокочастотные live-данные.

Для этого используются bounded queues, ring buffers, double buffers, latest-value buffers или другие специализированные каналы.

---

## 3.5. Definitions, options, state и persistence не смешиваются

Это четыре разных типа данных.

### Definitions

Неизменяемое описание возможностей устройства:

- типы фильтров;
- диапазоны gain, frequency и Q;
- единицы измерения;
- default values;
- parameter IDs;
- число банков;
- число фильтров;
- DSP topology;
- правила mapping.

```cpp
struct ParameterDefinition {
    ParameterId id;
    ValueRange range;
    MappingType mapping;
    double defaultValue;
    Unit unit;
};
```

Definitions должны находиться в типизированном коде, а не в runtime JSON.

### Options

Настройки поведения алгоритмов:

- FFT size;
- размер окна;
- число выходных bins;
- stream queue size;
- optimizer limits;
- smoothing;
- update rate;
- oversampling mode.

```cpp
struct AnalyzerOptions {
    std::size_t fftSize = 2048;
    std::size_t outputBinCount = 256;
    double updateRateHz = 20.0;
};
```

### State

Текущие значения устройства:

- gain фильтра;
- frequency;
- active bank;
- bypass;
- analysis status;
- progress;
- текущий candidate.

State принадлежит stores.

### External configuration

JSON остаётся только там, где данные действительно должны быть внешними:

- импорт и экспорт presets;
- пользовательские profiles;
- внешние device definitions;
- обмен с другими приложениями;
- debug dumps;
- migration tools.

JSON не используется как внутренний runtime-протокол.

---

# 4. Общая шина и формат сообщений

## 4.1. Max-шина остаётся транспортом

Существующую общую шину необязательно удалять.

Разрешённые направления:

```text
component → host
host → component
```

Запрещённая логическая модель:

```text
component → произвольный component
```

Даже если Max физически доставляет сообщение всем, протокол должен разделять:

- команды, адресованные Host;
- события и snapshots, опубликованные Host.

---

## 4.2. Runtime messaging переводится на Max atoms

Из внутреннего runtime-обмена удаляются:

- JSON-строки;
- `Dictionary` как payload;
- текстовая сериализация C++-объектов;
- динамический разбор вложенных конфигураций;
- передача имён Dictionary между компонентами.

Пример команды:

```text
command 1 ui eq.set_parameter bank 0 filter 2 gain -3.5
```

Пример события:

```text
event 1 store.updated eq 42
```

Где:

```text
command / event — категория сообщения;
1               — protocol version;
ui              — source;
eq              — домен или store;
42              — store revision.
```

Поле `source` можно сохранить для:

- логирования;
- диагностики;
- определения инициатора;
- сопоставления ответа с запросом.

Свободный `target` больше не нужен. Команда идёт Host, а Host сам определяет, какие компоненты должны получить событие или snapshot.

---

# 5. Messaging и Atom Codec

Следует создать отдельный слой:

```text
Consolidator.Messaging
    AtomReader
    AtomWriter
    CommandCodec
    EventCodec
    ProtocolValidator
    MessageLogger
```

Его обязанности:

- преобразовывать Max atoms в типизированные C++-команды;
- кодировать typed events обратно в atoms;
- проверять число и типы аргументов;
- проверять protocol version;
- поддерживать необязательные поля;
- возвращать понятные диагностические ошибки;
- не допускать невалидные команды до `DeviceHost`.

Пример typed command:

```cpp
struct SetEqParameterCommand {
    BankId bankId;
    FilterId filterId;
    EqParameter parameter;
    double value;
};
```

Пример декодирования:

```cpp
auto result = CommandCodec::decodeSetEqParameter(atoms);

if (!result) {
    reportProtocolError(result.error());
    return;
}

deviceHost.dispatch(result.value());
```

---

## 5.1. Три независимых вида версий

Нельзя смешивать следующие значения.

### Protocol version

Версия формата сообщения:

```text
protocolVersion = 1
```

Нужна для совместимости компонентов и Host.

### Store revision

Версия официального состояния конкретного store:

```text
eqRevision = 42
```

Увеличивается только после успешного commit.

### Operation или session ID

Идентификатор конкретного запуска длительной операции:

```text
fitSessionId = 17
```

Связывает start, progress, cancel, stale-result checking и final result.

Ни одно из этих значений не заменяет другое.

---

# 6. DeviceHost

`DeviceHost` — центральный объект каждого инстанса Consolidator.

Его обязанности:

- загрузить сохранённое состояние;
- владеть stores;
- принимать и маршрутизировать команды;
- валидировать изменения;
- выполнять атомарные commits;
- увеличивать revisions;
- публиковать события и snapshots;
- координировать workflows;
- управлять persistence;
- регистрировать компоненты;
- управлять sessions;
- вести диагностику.

```cpp
class DeviceHost {
public:
    void handle(const Command& command);
    void attach(const ComponentInfo& component);
    void detach(ComponentId componentId);

private:
    DeviceStores stores_;
    CommandRouter commandRouter_;
    WorkflowManager workflowManager_;
    NotificationPublisher notificationPublisher_;
    PersistenceService persistenceService_;
    ComponentRegistry componentRegistry_;
};
```

---

## 6.1. Host не должен становиться God Object

В `DeviceHost` не должно быть:

- DSP-формул;
- FFT-алгоритмов;
- реализации оптимизатора;
- знания конкретных `live.dial`;
- UI-рисования;
- ручного кода каждой многошаговой операции;
- огромного `switch` по всем сообщениям.

Команды обрабатывают специализированные handlers:

```text
EqCommandHandler
CompressorCommandHandler
SaturatorCommandHandler
AnalyzerCommandHandler
ApproximatorCommandHandler
PresetCommandHandler
UiCommandHandler
```

Многошаговые операции координируют workflows:

```text
FitWorkflow
CaptureWorkflow
BankSwitchWorkflow
PresetLoadWorkflow
ReferenceRecordingWorkflow
ApplyCandidateWorkflow
```

Host остаётся фасадом, владельцем состояния и инфраструктурным центром.

---

# 7. Stores

Состояние делится на логические домены:

```text
EqStore
CompressorStore
SaturatorStore
AnalyzerStore
ApproximatorStore
GlobalStore
```

Store имеет смысл создавать, если состояние:

- имеет самостоятельный жизненный цикл;
- обновляется как логическая единица;
- используется несколькими компонентами;
- требует собственной revision;
- может заменяться snapshot целиком;
- должно сохраняться или восстанавливаться независимо.

Необязательно создавать polymorphic store-класс для каждой мелочи.

---

## 7.1. Пример EqStore

```cpp
class EqStore {
public:
    const EqState& state() const noexcept;
    std::uint64_t revision() const noexcept;

    UpdateResult apply(const SetEqParameterCommand& command);
    UpdateResult apply(const SelectEqBankCommand& command);
    UpdateResult replace(const EqSnapshot& snapshot);

private:
    EqState state_;
    std::uint64_t revision_ = 0;
};
```

После успешного commit Host публикует:

```text
store.updated eq 43
```

При ошибке состояние и revision не меняются.

---

## 7.2. Revision увеличивается только после commit

Правильная последовательность:

```text
1. Получить команду.
2. Проверить формат и аргументы.
3. Провалидировать значения.
4. Построить новое целевое состояние.
5. Применить все связанные изменения.
6. Завершить commit.
7. Увеличить revision.
8. Опубликовать событие.
```

Подписчики никогда не должны видеть промежуточное состояние.

---

## 7.3. Транзакции

Массовые изменения применяются атомарно.

Например, смена банка может одновременно менять:

- параметры всех фильтров;
- bypass;
- выбранный банк;
- активные UI-контролы;
- DSP snapshot.

```text
begin transaction
replace filters
update bypass states
select active bank
validate complete state
commit
revision++
publish store.updated
```

Одиночное движение ручки считается транзакцией из одного изменения.

---

# 8. State snapshots

Snapshot — immutable-представление полного состояния store на конкретной revision.

```cpp
struct EqSnapshot {
    std::uint64_t revision;
    EqState state;
};
```

Snapshots нужны для:

- первоначальной синхронизации;
- подключения нового компонента;
- восстановления после загрузки;
- массовой замены состояния;
- безопасной передачи данных в DSP;
- диагностики;
- тестирования.

Компонент не должен собирать полный state из случайной последовательности событий.

Рабочая модель:

1. получить полный snapshot;
2. сохранить локальную immutable-копию;
3. затем применять incremental updates или запрашивать новый snapshot при несовпадении revision.

---

# 9. Persistence и Dictionary

`Dictionary` остаётся только внутренней деталью persistence.

Загрузка:

```text
Dictionary
    ↓
PersistenceCodec
    ↓ decode / migrate / validate
Typed stores in DeviceHost
```

Сохранение:

```text
Command
    ↓
DeviceHost
    ↓
Typed store commit
    ↓
PersistenceService
    ↓
Dictionary
```

`Dictionary` больше не является:

- message bus;
- runtime state API;
- общим shared storage;
- источником данных для DSP;
- способом связи UI, Analyzer и Approximator;
- объектом, который читают все компоненты.

---

## 9.1. PersistenceService

Отдельный сервис отвечает за:

- чтение и запись Dictionary;
- преобразование typed state;
- schema version;
- migrations;
- defaults;
- импорт и экспорт JSON;
- транзакционное сохранение;
- восстановление повреждённого или устаревшего состояния.

```cpp
struct PersistedDeviceState {
    int schemaVersion;
    EqSnapshot eq;
    CompressorSnapshot compressor;
    SaturatorSnapshot saturator;
    PersistentUiSnapshot ui;
};
```

Не нужно сохранять:

- hover;
- mouse drag;
- временную selection;
- текущую итерацию optimizer;
- audio buffers;
- FFT frame queue;
- worker thread handles;
- кратковременные UI-errors;
- transient progress, который не имеет смысла после перезапуска.

---

## 9.2. Компонентам не выдаются Dictionary readers

DSP, Analyzer, Approximator и UI не должны иметь API для чтения общего Dictionary.

Они получают только:

- atoms snapshot от Host;
- immutable typed snapshot;
- специализированный runtime API;
- подготовленные данные своей области.

Иначе Dictionary снова станет скрытым публичным API всей системы.

---

# 10. Инициализация

Все runtime features устройства статические, поэтому component registry и
startup handshake не нужны. Корневой patch после `deferlow` восстанавливает
persistence и один раз отправляет `persistence_ready`. Host после этого
публикует `host.initialized`, definitions, EQ snapshot и DSP snapshot.

Feature-контроллеры ничего не публикуют из `loadbang`. Для ручной диагностики
Host сохраняет `bang`, который повторно публикует текущее состояние, но этот
маршрут не участвует в обычной загрузке устройства.

---

# 11. DSP Engine

Разрозненные audio-компоненты объединяются в единый DSP Engine, который владеет полной цепью обработки.

Пример topology:

```text
Input
→ Input Gain
→ Pre EQ
→ Saturation
→ Compressor
→ Post EQ
→ Output Gain
→ Output
```

Фактический порядок может изменяться, но topology должна принадлежать одному DSP-компоненту.

---

## 11.1. Ответственность DSP Engine

DSP Engine:

- принимает аудиосигнал;
- получает готовые DSP snapshots;
- строит и обновляет processing chain;
- применяет параметры;
- обрабатывает аудио;
- безопасно меняет состояние на audio thread;
- не знает про UI;
- не читает Dictionary;
- не запускает workflows;
- не общается с Approximator напрямую;
- не владеет persistent state.

---

## 11.2. DSP snapshot

Host или отдельный builder формирует компактное состояние, нужное только DSP.

```cpp
struct DspSnapshot {
    InputGainState inputGain;
    EqDspState preEq;
    SaturatorDspState saturator;
    CompressorDspState compressor;
    EqDspState postEq;
    OutputGainState outputGain;
};
```

Передавать полный `DeviceState` в audio engine необязательно и нежелательно.

---

## 11.3. Безопасность audio thread

На audio thread запрещены:

- Dictionary;
- JSON;
- Max messages;
- mutex с потенциальным ожиданием;
- allocation внутри обработки блока;
- тяжёлая сериализация;
- optimizer;
- прямой доступ к UI;
- чтение изменяемого shared state.

Рекомендуемая модель:

```text
Main thread:
store commit
→ build immutable DspSnapshot
→ atomic pointer swap

Audio thread:
load snapshot pointer
→ process audio block
```

При необходимости параметры сглаживаются внутри DSP.

---

## 11.4. Разделение DSP-проектов

Общая математика выносится в:

```text
Consolidator.DspCore
```

Там находятся:

- biquad coefficients;
- bell, shelf и pass filters;
- transfer functions;
- magnitude response;
- compressor math;
- saturation models;
- oversampling utilities;
- parameter smoothing;
- filter-chain models.

Max-specific слой:

```text
Consolidator.DspExternal
```

содержит:

- Max external lifecycle;
- audio inlets и outlets;
- signal vector processing;
- snapshot swap;
- интеграцию с audio thread.

Это позволяет Analyzer, Painter и Approximator использовать общую математику без зависимости от Max audio external.

---

# 12. Analyzer

Analyzer становится независимым универсальным анализатором аудиопризнаков, а не только EQ-анализатором.

Его задача — описывать звучание main и reference сигналов через потоковые измерения, накопленную статистику и итоговый Feature Vector.

Analyzer может выполнять:

- приём main и reference audio;
- windowing;
- FFT;
- спектральные измерения;
- динамические и временные метрики;
- накопление статистики;
- формирование analysis frames и chunks;
- публикацию progress;
- формирование итогового `AnalysisResult`;
- передачу потоковых данных consumer-компонентам.

Analyzer не должен:

- изменять EQ;
- обновлять UI напрямую;
- писать Dictionary;
- знать внутренний алгоритм Approximator;
- решать, применять ли результат;
- хранить каждый FFT frame в store.

---

## 12.1. AnalyzerState

В `AnalyzerStore` хранится только устойчивое состояние операции:

```cpp
struct AnalyzerState {
    AnalysisStatus status;
    SessionId sessionId;
    std::uint64_t framesProcessed;
    double progress;
    SpectrumCurve latestCurve;
    std::optional<AnalysisResult> completedResult;
    std::optional<AnalysisError> error;
};
```

Высокочастотные frames передаются через stream, а в store попадают только агрегированные данные, progress и итоговый результат.

---

# 13. Approximator

Approximator — отдельный вычислительный компонент.

Его обязанности:

- принять запрос на fit;
- получить analysis data;
- запустить worker thread;
- выполнять оптимизацию;
- публиковать progress;
- формировать промежуточные candidates;
- поддерживать cancel;
- вернуть typed final result.

Approximator не должен напрямую:

- устанавливать фильтры в DSP;
- двигать UI controls;
- изменять `EqStore`;
- писать Dictionary.

Он возвращает Host результат:

```text
fit.completed session 17 ...
```

Host проверяет:

- совпадает ли session ID;
- не отменена ли операция;
- не является ли результат stale;
- валидны ли параметры;
- соответствуют ли они definitions;
- можно ли применить результат к текущему состоянию.

---

## 13.1. Candidate и официальный state

Необходимо разделять:

```text
FitCandidate
```

и:

```text
официальный EqState
```

Candidate можно показывать в UI и Painter, не меняя рабочую DSP-цепь.

```cpp
struct ApproximatorState {
    FitStatus status;
    SessionId sessionId;
    double progress;
    double currentLoss;
    std::optional<FitCandidate> latestCandidate;
    std::optional<FitResult> completedResult;
};
```

Только после завершения workflow, автоматического apply или подтверждения пользователя результат коммитится в `EqStore`.

---

# 14. Потоковые данные

Высокочастотные данные не передаются через stores и обычные atom events.

Для связи Analyzer и Approximator можно использовать:

- bounded SPSC queue;
- ring buffer;
- double buffer;
- latest-value buffer;
- typed shared stream;
- отдельный signal path, если передаётся аудио.

---

## 14.1. Host управляет жизненным циклом stream

Host:

- создаёт session;
- определяет producer и consumer;
- запускает workflow;
- передаёт endpoint или stream ID;
- завершает и очищает stream;
- обрабатывает cancel;
- фиксирует итоговый результат.

Host не должен проксировать каждый frame.

Плохо:

```text
Analyzer → Host → Approximator
```

для каждого FFT frame.

Хорошо:

```text
Host создаёт AnalysisStream
Analyzer пишет в stream
Approximator читает stream
Host получает progress и final result
```

---

## 14.2. Политика очереди

Если важен каждый frame:

```text
bounded SPSC queue
```

При переполнении заранее задаётся политика:

- отбросить старейший frame;
- отбросить новый frame;
- увеличить dropped-frame counter;
- остановить capture с ошибкой.

Бесконечная очередь недопустима.

Если нужен только последний frame:

```text
latest-value buffer
```

Старые значения можно пропускать.

Если Analyzer умеет агрегировать данные, предпочтительнее передавать редкие `AnalysisChunk`, а не каждый FFT frame.

---

# 15. Workflows

Workflow описывает многошаговую операцию и её жизненный цикл.

Компоненты не должны самостоятельно строить длинные цепочки реакций друг на друга.

Примеры:

```text
FitWorkflow
CaptureWorkflow
PresetLoadWorkflow
BankSwitchWorkflow
ReferenceAnalysisWorkflow
ApplyCandidateWorkflow
```

Workflow отвечает за:

- запуск;
- переходы состояний;
- progress;
- cancel;
- timeout;
- component missing;
- stale session results;
- partial failure;
- cleanup;
- повторный запуск.

---

## 15.1. FitWorkflow

Пример последовательности:

```text
1. UI отправляет fit.start.
2. Host создаёт session ID.
3. FitWorkflow переходит в Starting.
4. Analyzer получает команду начать capture.
5. Host создаёт AnalysisStream.
6. Approximator получает stream consumer.
7. Analyzer публикует AnalysisChunk.
8. Approximator выполняет fit.
9. Host получает progress events.
10. Approximator возвращает FitResult.
11. Host валидирует результат.
12. ApproximatorStore обновляется.
13. При auto-apply обновляется EqStore.
14. DSP и UI получают store.updated.
15. Workflow публикует operation.completed.
```

---

## 15.2. Workflow как state machine

```cpp
enum class FitWorkflowState {
    Idle,
    Starting,
    Capturing,
    Approximating,
    Validating,
    Applying,
    Completed,
    Failed,
    Cancelled
};
```

Явная state machine предпочтительнее набора несвязанных реакций на сообщения, потому что её легче тестировать, логировать и восстанавливать после ошибки.

---

# 16. UI Controller

Следует создать единый `DeviceUiController`, вероятно на JS.

Он заменяет существующие JS-компоненты, которые одновременно:

- хранят state;
- управляют банками;
- отправляют команды;
- синхронизируют controls;
- рисуют;
- читают Dictionary.

Новый UI Controller — адаптер между Host и UI controls.

---

## 16.1. Registry controls

```javascript
const controls = {
    "eq.bank.0.filter.2.gain": {
        scriptingName: "filter2Gain",
        command: "eq.set_parameter",
        parameter: "gain",
        writable: true
    },

    "eq.bank.0.filter.2.frequency": {
        scriptingName: "filter2Frequency",
        command: "eq.set_parameter",
        parameter: "frequency",
        writable: true
    }
};
```

Registry можно строить:

- статически в JS;
- по patcher scripting names;
- из arguments;
- из компактных atom definitions от Host.

Базовые device definitions при этом остаются в типизированном C++-коде, а не в JSON.

---

## 16.2. Ответственность UI Controller

UI Controller:

- слушает изменения controls;
- преобразует UI gestures в команды;
- отправляет команды Host;
- получает snapshots и events;
- обновляет controls подтверждёнными значениями;
- управляет enabled/disabled;
- переключает отображаемые банки;
- форматирует значения;
- подавляет feedback loops;
- хранит только локальное визуальное состояние.

Он не является владельцем EQ, compressor или других DSP-параметров.

---

## 16.3. Подавление feedback loop

Без защиты возможен цикл:

```text
Host
→ UI set value
→ control callback
→ Host command
→ Host update
→ UI set value
```

UI Controller должен различать:

```text
user-originated update
host-originated update
```

Простейший вариант:

```javascript
updatingFromHost = true;
setControlValue(id, value);
updatingFromHost = false;
```

Более надёжный вариант — revision или update token для подавления echo.

---

## 16.4. Локальное UI-состояние

Локально остаются:

- hovered handle;
- открытый popup;
- текущий drag;
- selection rectangle;
- tooltip;
- panel animation;
- mouse coordinates.

В store отправляются только устойчивые значения:

- frequency;
- gain;
- Q;
- selected bank;
- enabled state;
- bypass;
- подтверждённые настройки.

---

# 17. Painters

Painter-компоненты отвечают только за отображение.

Они получают подготовленные visual snapshots:

- individual filter curves;
- combined curve;
- colors;
- selection state;
- candidate curve;
- reference spectrum;
- transient UI highlights.

Painter не должен:

- владеть официальным EQ state;
- читать Dictionary;
- валидировать параметры;
- запускать Analyzer;
- менять DSP;
- координировать банки;
- принимать архитектурные решения.

Visual snapshot формирует Host, UI Controller или отдельный presentation builder.

---

# 18. Рекомендуемое разделение проектов

## Consolidator.Domain

Содержит:

```text
commands;
events;
IDs;
enums;
states;
snapshots;
results;
parameter definitions;
operation statuses.
```

Не зависит от Max.

## Consolidator.Messaging

Содержит:

```text
atom codecs;
protocol versioning;
message validation;
command parsing;
event encoding;
diagnostics.
```

## Consolidator.DeviceHost

Содержит:

```text
DeviceHost;
stores;
command handlers;
workflow manager;
component registry;
notification publisher;
state transactions.
```

## Consolidator.Persistence

Содержит:

```text
Dictionary codec;
JSON import/export;
schema version;
migrations;
defaults;
persistence validation.
```

## Consolidator.DspCore

Содержит:

```text
filter formulas;
transfer functions;
coefficient calculation;
compressor;
saturation;
oversampling;
parameter smoothing;
DSP models.
```

## Consolidator.DspExternal

Содержит:

```text
Max audio external;
audio lifecycle;
DspSnapshot swap;
signal processing;
audio-thread integration.
```

## Consolidator.Analyzer

Содержит:

```text
windowing;
FFT;
feature extraction;
statistics;
curve aggregation;
analysis sessions;
analysis stream producer.
```

## Consolidator.Approximator

Содержит:

```text
optimizer;
fit models;
candidate generation;
worker thread;
progress;
cancel;
fit result.
```

## DeviceUiController.js

Содержит:

```text
control registry;
UI event conversion;
snapshot application;
bank presentation;
feedback suppression;
enabled/disabled state.
```

## Painter JS components

Содержат:

```text
drawing;
curve visualization;
icons;
selection graphics;
candidate/reference display.
```

---

# 19. Типовые потоки

## 19.1. Изменение EQ-параметра

```text
1. Пользователь двигает control.
2. DeviceUiController определяет parameter ID.
3. UI Controller отправляет atom command Host.
4. Messaging codec декодирует command.
5. EqCommandHandler валидирует значение.
6. EqStore применяет изменение транзакционно.
7. EqStore увеличивает revision.
8. Host публикует store.updated eq <revision>.
9. Host или builder создаёт новый DspSnapshot.
10. DSP получает snapshot.
11. Painter получает visual snapshot.
12. UI получает подтверждённое значение.
13. PersistenceService сохраняет изменение при необходимости.
```

Ни DSP, ни Painter, ни UI Controller не пересылают параметры друг другу.

---

## 19.2. Смена EQ-банка

```text
1. UI отправляет eq.select_bank.
2. Host запускает handler или BankSwitchWorkflow.
3. EqStore валидирует bank ID.
4. Параметры, bypass и active bank заменяются одной транзакцией.
5. Revision увеличивается один раз.
6. DSP получает полный EQ snapshot.
7. UI Controller обновляет controls выбранного банка.
8. Painter перерисовывает кривые.
9. PersistenceService сохраняет банк и его параметры.
```

Не нужно отправлять десятки независимых сообщений между stores, DspProcessor и controls.

---

## 19.3. Live fit

```text
1. UI отправляет fit.start.
2. Host создаёт session ID.
3. FitWorkflow запускает Analyzer.
4. Host создаёт AnalysisStream.
5. Analyzer пишет AnalysisChunk в stream.
6. Approximator worker читает chunks.
7. Approximator публикует progress Host.
8. Host обновляет ApproximatorStore.
9. UI показывает progress и candidate.
10. Approximator возвращает FitResult.
11. Host проверяет session ID и валидность.
12. ApproximatorStore получает final result.
13. При apply Host обновляет EqStore.
14. DSP получает новый snapshot.
15. UI и Painter синхронизируются с EqStore.
16. Host публикует operation.completed.
```

Поток анализа не записывается покадрово в store.

---

# 20. Что удалить или заменить

Во время рефакторинга следует постепенно удалить:

- Dictionary как message payload;
- JSON как внутренний транспорт;
- прямые component-to-component commands;
- свободную модель `source-target`;
- самостоятельное чтение Dictionary компонентами;
- несколько независимых полных caches состояния;
- глобальное бессодержательное `device_state_updated`;
- ручные readiness-цепочки;
- UI-компоненты, одновременно являющиеся store;
- DSP-компоненты с отдельными системами состояния;
- дублирование filter definitions в JSON и JS;
- текстовую сериализацию объектов для внутреннего обмена;
- передачу каждого live-frame через Host.

Заменить на:

```text
typed commands;
typed events;
Atom Codec;
DeviceHost;
domain stores;
scoped revisions;
immutable snapshots;
state transactions;
workflows;
specialized streams;
single DSP Engine;
DeviceUiController;
PersistenceService.
```

---

# 21. План рефакторинга

## Этап 1. Domain types

Создать:

- IDs;
- enums;
- parameter definitions;
- state structures;
- snapshots;
- commands;
- events;
- operation statuses.

На этом этапе существующий Max patch можно не менять.

---

## Этап 2. Atom messaging

Создать новый Atom Codec.

Начать переводить сообщения по одному. На переходном этапе старый и новый протоколы могут работать параллельно.

---

## Этап 3. Минимальный DeviceHost

Реализовать:

- component attach;
- `EqStore`;
- command routing;
- store revision;
- `store.updated`;
- выдачу snapshot.

Сначала перевести один небольшой поток, например изменение gain одного EQ-фильтра.

---

## Этап 4. UI Controller

Создать registry controls и перевести EQ UI на модель:

```text
UI command → Host → EqStore → UI snapshot
```

После этого старый `EqStore.js` удалить либо сократить и переименовать в `DeviceUiController`.

---

## Этап 5. Persistence

Сосредоточить весь доступ к Dictionary в `PersistenceService`.

Удалить Dictionary readers из DSP, Analyzer, Approximator и UI.

---

## Этап 6. Unified DSP Engine

Объединить audio processors в единый DSP external.

Добавить immutable `DspSnapshot` и безопасный snapshot swap.

---

## Этап 7. Analyzer и Approximator

Сначала перевести их команды, progress и final results через Host.

Затем создать отдельный `AnalysisStream` для live-data.

Analyzer при этом рефакторится в универсальный Feature Extractor.

---

## Этап 8. Workflows

Вынести в явные state machines:

- fit;
- capture;
- reference analysis;
- preset load;
- bank switching;
- apply candidate.

---

## Этап 9. Удаление legacy

После миграции удалить:

- старый message protocol;
- direct routing;
- runtime Dictionary messages;
- дублирующие state caches;
- старые ready/start chains;
- старые DSP state managers;
- временные compatibility adapters.

---

# 22. Итоговая модель

```text
Bus
    Физический транспорт внутри Max.

Atoms
    Формат команд, событий и snapshots.

DeviceHost
    Центральная точка коммуникации, валидации и commit.

Stores
    Единственный источник актуального состояния.

Revisions
    Версии отдельных stores после успешного commit.

Snapshots
    Immutable-состояние store на конкретной revision.

Transactions
    Атомарное применение связанных изменений.

Workflows
    Координация многошаговых операций.

Streams
    Прямые специализированные каналы live-данных.

DSP Engine
    Единая аудиоцепь с безопасным snapshot swap.

Analyzer
    Универсальный анализ и производство feature/analysis data.

Approximator
    Вычисление candidates и final fit result.

DeviceUiController
    Адаптер между Host и UI controls.

Painters
    Только визуализация.

Dictionary
    Только persistence.

JSON
    Только импорт, экспорт и внешняя конфигурация.

Definitions
    Типизированное описание возможностей устройства в коде.
```

---

# 23. Краткий свод правил

```text
1. Компоненты не знают друг о друге.
2. Обычная коммуникация проходит только через DeviceHost.
3. Только Host изменяет официальное состояние.
4. Команда выражает намерение, событие — подтверждённый факт.
5. Store revision увеличивается только после успешного commit.
6. Связанные изменения применяются одной транзакцией.
7. Компоненты синхронизируются через snapshots, а не собирают state из случайных сообщений.
8. Dictionary не используется как runtime state API.
9. JSON не используется как внутренний протокол.
10. DSP не читает Dictionary и не обрабатывает Max messages на audio thread.
11. Live-data передаётся через специализированные bounded streams.
12. Host управляет жизненным циклом stream, но не проксирует каждый frame.
13. Итоговые устойчивые результаты коммитятся в stores.
14. Candidate не равен официальному DSP state.
15. UI отправляет намерения, но отображает подтверждённые значения.
16. Painters только рисуют.
17. Сложные операции реализуются как workflows/state machines.
18. Definitions, options, state и persistence существуют отдельно.
19. Общая Max-шина остаётся транспортом, но не свободной сетью общения.
20. DeviceHost остаётся фасадом и координатором, а не God Object.
```
