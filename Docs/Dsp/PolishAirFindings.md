# Air / Revival Shimmer: полезные выводы

- Reference rack измерять с solo нужного слота и Thickness0. Иначе в результат
  попадает окраска других модулей. Shimmer0/10/20/30/40 сопоставлялись Air0/25/50/75/100%.
- Neutral baseline делить из complex fundamental response; harmonic powers
  вычитать по порядкам и рассчитывать THD заново, не вычитать dB напрямую.
- Линейная часть лучше описывалась параллельными HP additions, чем одним high shelf.
  Рост H3 указывал на cubic source, но не доказывал внутреннюю схему Revival.
- Линейная и нелинейная ветки по-разному зависят от уровня. Peak hold предотвращает
  искусственную harmonic comb на stationary tone.
- Короткие sine-окна принимали leakage за гармоники: минимум12 preroll periods,
  20 measurement periods и significance floor.
- Исследовались преимущественно settled44.1 kHz probes. Это не доказательство
  соответствия на всех rates, transients, stereo и automation.

[Текущий алгоритм](PolishAirProcessor.md). Исходные таблицы измерений удалены.
