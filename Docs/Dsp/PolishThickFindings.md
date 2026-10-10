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

[Текущий алгоритм](PolishThickProcessor.md). Промежуточные оптимизации и отчёты удалены.
