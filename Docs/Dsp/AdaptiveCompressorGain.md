# Auto Gain и видимый Output

## Управление

Универсальный DeviceCompressor имеет Auto Gain, включённый по умолчанию. Он плавно
управляет единственным Output −24…+24 дБ. Отдельной скрытой адаптивной поправки и
ограничения ±3 дБ больше нет.

```text
dry input → Normal-модель и её статический makeup → Output → выход
    └──── входной измеритель           └──── измеритель до Output
```

Статическая компенсация остаётся частью Normal-моделей. Галочка управляет именно
измеряемым автогейном. Индивидуальные Native-модели и Emulation не меняются.

- ON: DSP меняет Output, VST3 обновляет реальный host parameter и ручку.
- OFF: достигнутый Output сохраняется и далее регулируется вручную.
- Ручное изменение Output, включая host automation, отключает Auto Gain.
- Bypass обходит процессор и замораживает histories.

Начальное значение Output — 0 дБ. Актуальный preset сохраняет Auto Gain и Output;
после загрузки измеритель переоценивает уровень по новому входу. State требует все
восемь текущих контролов; старые семипараметровые states не мигрируются.

## Измерение и паузы

Два независимых измерителя используют K-weighting по
[ITU BS.1770](https://www.itu.int/rec/R-REC-BS.1770). Фильтры high shelf/RLB находятся
только в измерителях и компилируются для sample rate. Энергия выбранных каналов
усредняется; один общий Output сохраняет stereo.

Энергия 20-мс интервалов экспоненциально усредняется с постоянной времени 3 с.
Это оценка K-weighted энергии, не полный EBU R128 meter со скользящим окном/gate.

```text
targetOutputDb = clamp(10*log10(dryMeanPower/preOutputWetMeanPower), -24, +24)
outputDb += coefficient4seconds * (targetOutputDb - outputDb)
```

При ошибке ≤.3 дБ регулировка не обновляется. Gain сглаживается 10-мс ramp;
первые .5 с активных интервалов отведены на накопление энергии. Четыре секунды —
постоянная времени, не время полного установления.

На входной K-weighted mean-square ниже −60 дБ либо на 20 дБ ниже недавней средней
энергии входа Output и его ramp замораживаются. Это gate измерителя, не noise gate
аудио. Средняя энергия продолжает обновляться в паузах: следующий тихий источник
не блокируется навсегда. Нулевое молчание остаётся закрытым независимо от длительности;
деления на нулевую wet energy нет.

После смены алгоритма измерение начинается заново, Output сохраняется. OFF не
запускает weighting/energy measurement. Включение начинает с текущего Output.
Lookahead/limiter нет; краткие пики не ограничиваются, медленная динамика немного меняется.

## Native и host publication

Код — Processors/DeviceCompressor/AdaptiveCompressorGain.h/.cpp. Модели получают
Output=1.0, единственный физический gain применяется этой стадией. OutputDb()
читает текущий сглаженный gain на audio thread.

VST3 сохраняет его в atomic snapshot после блока. Processor timer на message thread
20 раз/с публикует Output через setValueNotifyingHost; SliderAttachment двигает ручку.
Timer работает и при закрытом editor. DSP не ждёт UI и не зависит от таймера.
Если offline host не обслуживает message loop, звук корректен, но host/UI snapshot
обновляется только при обслуживании сообщений.

Собственные публикации отличаются от ручных изменений с учётом float-нормализации
JUCE/APVTS. Ручное изменение сразу переводит audio path в manual; checkbox
публикуется на message thread. Выключение checkbox переносит последний audio
snapshot в Output. Host самостоятельно решает, записывать ли это в automation.
На audio thread нет parameter gestures/host notifications.

Restore preset и destructor исключают конкурирующую timer publication через
MessageManagerLock вне realtime. Timer остановлен до уничтожения параметров.
Prepare выделяет dry power buffer размером maximumFrameCount, компилирует фильтры.
Process не выделяет память, не блокируется и не выполняет I/O/Managed calls.
Контракт: audio sample rate, capacity>0, frameCount≤capacity. Tiny meter states
очищаются на control rate против denormals. ABI/runtime graph не менялись.

## Ручное исследование

Восемь ненормализованных записей, шесть наборов controls, четыре алгоритма:
192 WAV, 44.1 kHz / block 128, Auto Gain ON, начальный Output=0 дБ. В каждом
из 48 сравнений измерен max LUFS минус min LUFS четырёх алгоритмов.

| Разброс, дБ | Только статический makeup | Текущий Auto Gain |
| --- | --- | --- |
| Средний | 1.61 | 0.88 |
| Медиана | 1.40 | 0.62 |
| 90-й процентиль | 3.17 | 1.95 |
| Максимум | 5.72 | 3.97 |

Фактический Output на записях −4.814…+4.510 дБ. Ещё 64 прохода случайных синусов,
импульсов/пауз при 44.1/96 kHz: NaN/Infinity нет, Output −1.410…+16.902 дБ,
в последней секунде тишины samples нулевые. На тихом 1 kHz после громкого измеренное
движение Output меньше .000003 дБ (float32 погрешность сравнения).

Probe: 60 с синуса 1 kHz, peak −48 dBFS, затем 5 с тишины. При Reduction=1 и
4:1 итоговый Output 76/3A/2A/SSL примерно −21.57/−15.73/−16.99/−8.54 дБ.
Последняя секунда синуса отличается от dry RMS на .299 дБ — намеренный deadband;
предел ±3 дБ не мешает регулировке. OFF и ручные 0/+12 дБ дают ожидаемое отношение
amplitudes, relative RMS error около 3.6e−8 для четырёх алгоритмов.

Metadata подтвердили Auto Gain default=ON и восемь контролов. UI publication,
manual takeover/reset/destruction проверены по исходникам и контракту JUCE.
Визуальное движение slider/live automation в DAW и прослушивание не проверялись.
Native и VST3 Release собраны и установлены; DSP-автотестов нет. Исследования остаются
в ignored build: удаление таких папок ранее блокировалось политикой выполнения команд.