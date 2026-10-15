# LevelerProcessor

[Исходник](../../Audio/Algorithms/Leveler/LevelerProcessor.cpp).

Один алгоритм используется независимыми Input/Output GainStage. Управляющий путь:
detector EQ -> KWeighting -> linked mean-square followers -> gain controller.
KWeighting принадлежит Primitives/Filters и фильтрует только управляющий сигнал.

Время усреднения/регулирования и gate остаются прежними. Уровень определяется как
10*log10 среднего K-взвешенного квадрата выбранных каналов. Нет BS.1770 channel sum,
смещения -0.691 или programme gate; это K-взвешенный детектор, не стандартный LUFS meter.
Основа frequency weighting: [ITU-R BS.1770](https://www.itu.int/rec/R-REC-BS.1770).
K-фильтры пересчитываются в Prepare и очищаются в Reset и при включении Leveler.

Slow energy: 1.5 с; slow gain controller: 6 с.

Fast energy: 250 мс; fast correction поверх slow: ±6 дБ, 400 мс вниз/800 мс вверх.

Коррекция равна разнице slow/fast уровней и возвращается к нулю на стационарном источнике.

Detector gate: 50 мс energy, open 1.9952623149688787e-6, close 1e-6, hold 200 мс.
Это прежние численные пороги (-57/-60 dB), теперь применённые к K-взвешенной энергии.

Он останавливает обучение, не обрезает аудио.

При закрытии slow gain удерживается, fast возвращается к нулю.

Включение/выключение начинает переход с текущего gain.

Ручной Input Level и Target сглаживаются 10 мс; внутри блока gain интерполируется геометрически.

Settled manual gain переводится в linear один раз на блок.
