# LevelerProcessor

[Исходник](../../External/Dsp/LevelerProcessor.cpp).

Связанный RMS выбранных каналов снимается до Input Gain.

Slow energy: 1.5 с; slow gain controller: 6 с.

Fast energy: 250 мс; fast correction поверх slow: ±6 дБ, 400 мс вниз/800 мс вверх.

Коррекция равна разнице slow/fast уровней и возвращается к нулю на стационарном источнике.

Detector gate: 50 мс energy, open −57 dBFS, close −60 dBFS, hold200 мс.

Он останавливает обучение, не обрезает аудио.

При закрытии slow gain удерживается, fast возвращается к нулю.

Включение/выключение начинает переход с текущего gain.

Ручной Input Level и Target сглаживаются 10 мс; внутри блока gain интерполируется геометрически.

Settled manual gain переводится в linear один раз на блок.
