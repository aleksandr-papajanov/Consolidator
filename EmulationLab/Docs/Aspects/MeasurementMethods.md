# Интерпретация измерений

## Что учитывать

- Проверить actual control readback и neutral baseline; rack reference измерять с solo
  нужного модуля. Sidechain HP и output HP имеют разные роли.
- Форму и громкость сравнивать отдельно: удалить средний dB offset по файлу/удару
  для shape, отдельно показать RMS/gain bias/peak. Один scalar score скрывает регрессии.
- Attack effect сравнивать с Attack0 при одинаковых Sustain/Limit.
- Всегда указывать окно1/2/5/20 мс и привязку к dry onset. Power release и amplitude
  release не равны. Carrier zero crossings не описывают gain envelope.
- Проверять cold start, quiet/scaled copies, слабый удар после сильного, stationary
  tone и down-step. Prepare очищает histories; continuous render их сохраняет.
- Settled sine: минимум12 preroll periods, 20 measured periods, Hann projection и
  significance floor. Leakage короткого окна нельзя принимать за гармоники.
- capture/probes учитывают reported latency; render-wav — нет. В тишине gainDb=null,
  а не measured0 дБ. Не смешивать разные методы выравнивания.
- Из complex baseline делить fundamental; harmonic powers вычитать по порядкам и
  заново рассчитывать THD. Paired output differences не доказывают внутреннюю wet-ветку.
- LinearAnalysis динамического плагина зависит от уровня и истории. Sine response
  не доказывает внутреннюю схему фильтров, duck или compressor reference.
- Whole-process CPU включает host/I/O/timer noise. Использовать длинные одинаковые
  renders, alternating order и bypass subtraction; это не callback timing Plugin Doctor.
- Загруженный VST3 нельзя заменить: сохранить проект, закрыть host, установить готовую
  сборку. После изменения parameter ranges использовать свежий экземпляр; старые
  normalized automation могли иметь другое значение. Не вводить aliases/migration.
- После выбора алгоритма оставить defaults в коде и короткие выводы в документе
  процессора; удалить временные отчёты, fitting scripts и processed WAV.

Reusable scripts: EmulationLab/Scripts/CompareTailBands.ps1 сравнивает полосы/окна,
RenderBandpass.ps1 фильтрует aligned stereo для прослушивания. Это offline инструменты,
не дополнительная realtime-топология.


Stepped-sine probe v3 сохраняет также harmonic.N.real/imag: комплексную гармонику,
нормированную к модулю выходного fundamental и повернутую на -N*phase(input fundamental).
Это сохраняет знак/фазу гармоники. Одних dBc недостаточно для выбора полярности
нелинейного компонента. При смене probe version заново снимать обе стороны; старые
amplitude-only datasets не превращать в измерения комплексных гармоник.
