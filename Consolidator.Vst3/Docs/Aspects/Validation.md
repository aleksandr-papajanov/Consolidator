# VST3 lifecycle и диагностика

Source review, component/ABI tests, сборка и реальное поведение в Live — разные
уровни доказательств. Сборка, проверки и ручной запуск требуют разрешений из
[AGENTS.md](../../../AGENTS.md); DSP-автотесты запрещены. Здесь описаны сценарии,
а не результаты последнего запуска или обязательный аудиотестовый gate.

## Если интерфейс не готов

Успешная Managed registration доказывает загрузку runtime, а видимые JUCE controls —
создание view. Ни то ни другое не доказывает, что принят authoritative snapshot.
Проследить initialize → foreground focus/activation → set_instance_active acknowledgement
→ registry/observe_target → полная принятая проекция → включение bindings.
Inactive view не должен начинать редактирование или применять поздний snapshot.

Проверять selector/version, request и target generation по текущему
[wire contract](../../../Consolidator.Managed/Docs/Aspects/ManagedProtocol.md).
FFT version 2 декодируется отдельно от command/notification version 1.
Временную диагностику удалять после выяснения причины; не оставлять её как
fallback и не менять обработку протокола на основании непроверенной гипотезы.

## UI, state и lifecycle

- Фокус/Inactive overlay, открытие и закрытие view при живом processor.
- Snapshot assembly без промежуточного включения bindings и лишних rebuilds;
  устаревшие replies не меняют target. Rejected edits восстанавливают принятую проекцию.
- Scalar/graph gestures, финальная позиция и acknowledgement, быстрое начало
  следующего жеста, закрытие с pending writes, reset и Undo/Redo.
- Local/group edits с эффективными ranges; независимые уровни участников при
  выключении Leveler; reset/freeze и project reload через Managed persistence.
- Group/Ungroup/Clear: одна атомарная Managed операция, автоматическая группа
  неизменяема. Обычный bank click очищает прежний явный выбор; Shift selection
  сохраняет правило одного банка на instance и посещает каждую ячейку один раз.
- Registry revision gaps вызывают один resync; telemetry revisions и levels
  обновляются независимо от topology. Malformed batch не применяется частично.
- Accepted bypass, временные overrides и action flashes; подтверждение очищает
  совпавший preview. Reset не создаёт дополнительный frontend history point.
- Все пять target contexts, четыре analyzer sources, sidechain/group/all-bank
  overlays, сохранение gray bypassed shapes, zoom/hit testing и collapsed Track List.
- Host names, удаление текущего target, sample-rate/block-size changes,
  save/load/duplicate и повторное открытие editor.

## Нагрузка и остановка

Для отдельно разрешённого host stress pass использовать 1/8/16/32 instances:
открывать/закрывать editor, редактировать EQ и группы, менять банки, делать Undo/Redo,
добавлять/удалять устройства и наблюдать 10–15 минут playback. Сравнивать queue
depth, control latency, analyzer activity и runtime metrics до/после; не должно
накапливаться устаревшее presentation/audio-analysis work.

Client fixtures должны повторять реальную форму protocol frames и asynchronous
completion, а host validation — реальный JUCE/DAW lifecycle. Callback, timer или
subscription не должны обращаться к уничтоженной view; widgets уничтожаются
раньше заимствованных models. После unregister callback невозможен.

Частоты visual repaint, coalesced control writes и Managed FFT независимы:
повышение плавности preview не увеличивает частоту команд или анализа.
FFT/telemetry используют replaceable latest slots, control frames — FIFO.
Контракты: [Host](Host.md), [Architecture](FrontendComposition.md),
[AnalyzerRendering](AnalyzerRendering.md), [RuntimeMetrics](../../../Consolidator.Managed/Docs/Aspects/RuntimeMetrics.md).
