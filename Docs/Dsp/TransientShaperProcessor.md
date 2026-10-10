# TransientShaperProcessor — общий Native / VST3 DSP

[Алгоритм](../../Consolidator.Native/External/Dsp/TransientShaperProcessor.cpp),
[настройки](../../Consolidator.Native/External/Dsp/TransientShaperProcessor.h),
[адаптер](../../ModelPlugins.Vst3/Source/TransientShaperPluginProcessor.cpp).
Это единственная реализация транзиент-шейпера. Native Compressor и VST3 компилируют
один TransientShaperProcessor.cpp и вызывают один Process. JUCE-адаптер хранит только
параметры хоста, state и заранее выделенные float/double буферы. Старый Native
алгоритм и отдельный AttackSustainMaskModel удалены.
Attack/Sustain: −1…+1, 0 нейтрально. Output Gain — ручной, default0 дБ. Limit постоянный.

## Детекторы и маски

Linked stereo detector — максимум модуля исходного сигнала после HP20, Q0.71.
Onset power follower: attack0.2 мс/release3 мс; context8.885735 мс.
Отношение onset/context создаёт continuous novelty, rise scale12.564163 дБ.
Novelty взвешивается текущим модулем относительно held peak.
Абсолютного порога dBFS и бинарной event/rearm логики нет.

Attack mask открывается с постоянной10 мкс, спадает13.519277 мс.
Attack addition mask содержит90% исходной маски и10% её decaying peak с постоянной60 мс.
Расширение не управляет Sustain и остаётся тёплым при Attack0.
Sustain mask=(1−AttackMask)*activity; activity — степень отношения fast power к held peak²,
exponent0.062542455. Открытие1.137880 мс, закрытие24.749357 мкс.

Fast power:3.447763/16.644667 мс; slow power:2.835169/567.710586 мс.
Held peak release270.620247 мс. Slow power начинается с −20 дБ, другие followers с нуля.
Slow/fast используют исходную, не обрезанную маской энергию.

```text
strength = clamp(1 + peakDb/116.891956, 0, 1)
tail = 0.122440108 + 0.885334152 * max((slow/fast)^(0.425903338*strength) - 1, 0)
```

## Единая signed gain law

```text
FA = 1 + 4.736178853 * AttackAdditionMask
FS = min(maximumGain, baseGain + SustainMask * tail)
GS = FS ^ SustainAmount
G  = clamp(GS * FA ^ AttackAmount, 1/maximumGain, maximumGain)
```

BaseGain соответствует −4.55 дБ; maximumGain — +36 дБ. Общий gain ограничен ±36 дБ.
Amount0 даёт unity, +1 полное действие, −1 обратный коэффициент.
Отрицательный amount не переворачивает полярность: gains строго положительны.
Negative Sustain обращает и снижение базы, и подъём хвоста; начало может стать громче.
Обе ручки имеют10 мс linear amount ramps, включая переход через ноль.

Порядок: original*GS → clamp±1 → Attack gain G/GS → HP10 → Output Gain → clamp±1.
Bypass возвращает raw audio. Limit-параметра и пути без Limit нет.
Фильтрация/клиппинг не сохраняют точную обратность уже обработанных WAV.

## Реализация

21 tuning controls считываются в Prepare. States/gain math — double; буферы выделяются
заранее, realtime не содержит allocation, locks, I/O или FFT. Peak dB пересчитывается
при новом peak, иначе продвигается точным dB decay. Степени с заранее известным
clamped результатом не вычисляются. Amount0/±1 имеют точные короткие расчёты;
±0.5 используют sqrt/reciprocal sqrt, остальные значения — pow. LUT, численных
аппроксимаций и пониженной частоты управления нет.

Normalized Attack/Sustain в VST3=(amount+1)/2:0.5 означает нейтральную ручку.
Старая normalized automation имела другой диапазон. Для обновлённого range нужен
свежий экземпляр. [Полезные выводы](TransientShaperFindings.md).

## Интеграция и lifecycle

Native SetParameters читает compressorAttack, compressorSustain и compressorOutputDb.
Managed физический диапазон Attack/Sustain −1…+1; default 0. Compressor Output
остаётся −36…+36 дБ; VST3 Output Gain −12…+12 дБ. Output Gain в обоих случаях
попадает в общий процессор после HP10, перед финальным clamp. Общий DspParameterRamp
интерполирует его линейный коэффициент за 10 мс, продвигаясь перед текущим sample.
Это заменяет JUCE SmoothedValue: длина рампы теперь round(fs*0.01), как в Native,
вместо JUCE floor(fs*0.01); при стандартных 44.1/48/96 кГц длина одинаковая.

Prepare компилирует коэффициенты и сбрасывает состояния; Reset сохраняет targets
ручек и устанавливает их как текущие значения. SetTuningBeforePrepare применяется
только перед Prepare. Native использует defaults header; VST3 передаёт те же
настройки через 21 prepare-only tuning control, хранимый хостом как float.

Process(DspProcessingFrame&) только выбирает audio/channel mode и вызывает общий
Process(DspAudioBuffer, mode). Stereo связывает детектор по максимальному модулю;
Left/Right учитывают и обрабатывают только выбранный ненулевой буфер. Другой канал
остаётся нетронутым. Внешний detector EQ Compressor используется компрессией;
transient detector получает свой вход после compression/makeup и собственный HP20.

Внутри активного Compressor маски и фильтры работают также при Attack=Sustain=0;
нулевые ручки дают unity gain law, но HP10 и постоянные clamp остаются.
Графовый bypass/activity/parallel относится ко всему Compressor и управляется
DspParallelStage; в отключённом VST3 bypass возвращает исходный сигнал. Полный
Native граф содержит другие устройства и потому не равен изолированному VST3.
Для сопоставимого сигнала отключить Compression, Parallel и остальные устройства.

Объединение выполнено по исходникам; сборка и новые аудиоизмерения не выполнялись.
