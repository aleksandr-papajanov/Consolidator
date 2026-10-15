# Consolidator.Managed — правила

Единственный владелец состояния и прикладного поведения Consolidator.
NativeAOT DLL обслуживает зарегистрированные host instances; UI и DSP получают
подготовленные данные, а не доступ к Managed state tree.

## Зона ответственности

- Декодировать и проверять команды; разрешать выбранный target и группу.
- Создавать StateValues с defaults, ranges, edit policy и участием в истории.
- Выполнять групповые и зависимые изменения параметров атомарно через существующие
  state transactions/observers. Вести общую историю и Undo/Redo.
- Владеть topology, audibility, lifecycle, persistence и строгим restore.
- Проецировать state в DspRuntimeState и публиковать фиксированный DSP snapshot.
- Принимать main/sidechain audio в bounded capture; выполнять FFT, анализ и
  аппроксимацию вне audio thread. Отправлять snapshots/notifications в frontend.

VST3 сообщает, что пользователь изменил значение или начал/закончил жест.
Какие peers затронуты, нужна ли история, какие внутренние параметры изменятся
и что попадёт в DSP — решает Managed. Эти последствия должны работать через
Managed-команды без специальной бизнес-логики frontend.

Новые значения создавать через действующие definitions/factory/composition с
observers, peers и effective ranges. Зависимые значения каждого участника группы
готовить в одной транзакции; frontend не вычисляет их и не отправляет обходные
writes. Persistence, reset и history policy подключать вместе с параметром,
когда они входят в его контракт, даже если звуковая реализация отложена.

Managed не рисует UI и не исполняет realtime audio graph. Его realtime audio
entrypoint только принимает данные в заранее подготовленный bounded transport.


Общие требования — [Rules](../../Docs/Rules.md). Техническое устройство проекта — [Architecture](Architecture.md).

## Dependencies и execution context

Dependencies передавать через constructor injection; Service Locator внутри
application/domain code запрещён. Generic State не зависит от Core/topology/
protocol/DSP. NativeAOT exports остаются thin boundary, без business logic.

Обычные collections, LINQ, DI, tasks и async/await допустимы на control/worker
path. Audio entrypoint следует realtime contract. Mutex/queue/worker вводить
только по конкретной concurrency requirement с явными producer, consumer,
lifetime и overflow policy.
