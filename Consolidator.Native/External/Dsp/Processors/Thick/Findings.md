# Thick: полезные выводы

- Собственная harmonic-enhancement схема, не точная копия R-Bass.
- Low-frequency fundamental окраску полезно отделять от генерации гармоник.
  Нормализация HF leakage превращала её в независимое от уровня искажение;
  нужны full-band level и bass-presence weighting.
- Rich odd series и low-level/sub harmonics используют разные ветки.
  Crest controller уменьшает добавку на ударах, не компрессируя dry input.
- Folding высоких порядков преднамеренно создаёт плотную окраску.
  Добавление oversampling или подавление всех aliases изменит звук.
- Source60 Гц не эквивалентен reference Freq и не отслеживает pitch.
- Final ceiling и fundamental leakage меняют measured linear gain на большом уровне.
  Sine harmonic fit не описывает полифонию, удары и intermodulation.

## Финальное упрощение

Семь level anchors заменены рациональной функцией. Конечный H2..H159 ряд
сохранён полностью, но считается через геометрическую сумму и семь Chebyshev
doubling steps. Body bells округлены; дополнительных веток нет.
Общие normalization/crest величины вычисляются один раз на sample.

Ручное сравнение с предыдущей моделью: Amount0/0.25/0.5/0.75/1,16-секундная
запись test1.wav и24-секундные тоны60/120/1000/12000 Гц,−30/−18/−12/−6/0/+6 дБFS,
44.1/96 кГц,block128. Максимальная RMS-разница относительно прежнего выхода:
0.066% на записи,0.415% на тонах; максимальный settled RMS level delta0.095 дБ.
Amount0 совпал с прежним dry. CPU-процент не измерялся; это сравнение моделей,
не доказательство точной эмуляции или слышимой эквивалентности.
Release VST3 и Native external собраны и установлены; source review не выявил
новых ownership/realtime проблем. DSP-автотесты не создавались и не запускались.

[Текущий алгоритм](README.md). Результаты не являются DSP-автотестами.
