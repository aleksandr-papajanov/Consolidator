# Realtime boundary

## Audio thread

Audio callback является realtime-critical.

В audio callback запрещены:

- allocations;
- `new`;
- mutex;
- blocking synchronization;
- waits;
- file/network I/O;
- logging;
- string operations;
- host/UI message output;
- FFT;
- optimization;
- general Managed application logic.

Разрешены:

- native DSP;
- чтение заранее опубликованного DSP snapshot;
- atomic operations;
- копирование в заранее выделенный audio buffer;
- короткий realtime-safe interop call.


## Managed audio input

Если аудио передаётся Managed для анализа, exported Managed audio entrypoint должен быть realtime-safe.

Он может:

- скопировать samples в заранее выделенный buffer;
- записать данные в bounded ring buffer;
- обновить atomic metadata.

Он не должен:

- выполнять FFT;
- выполнять analysis;
- аллоцировать память;
- использовать DI-driven application logic;
- логировать;
- блокироваться.

Analysis выполняется позже на Managed worker thread.


## DSP state

Managed application state не читается напрямую из audio thread.

Managed публикует подготовленный DSP state через realtime-safe boundary.

Предпочтительная модель:

```text
Managed state
 ↓
compile DSP snapshot
 ↓
prepared snapshot / triple-buffer atomic publish
 ↓
Native audio thread
 ↓
copy latest snapshot
 ↓
DSP
```

Через realtime boundary проходят только простые runtime-данные, необходимые DSP.

Native DSP не должен знать о:

- banks;
- groups;
- routing;
- UI state;
- command model;
- persistence.

