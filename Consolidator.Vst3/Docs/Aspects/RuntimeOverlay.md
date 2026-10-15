# Runtime Gain Staging overlays

GainStagingBindings владеет четырьмя RuntimeDialViewModel, привязанными к Input Level,
Input Target, Output Level и Output Target. ParameterBindings получает набор overlays;
каждая привязка заимствуется controls и живёт дольше них.

Принятый input_leveler/output_leveler определяет пару: при false Level редактируется,
а Target read-only и показывает RMS; при true Target редактируется, а Level read-only
и показывает slow gain. В автоматическом режиме fast correction отображается
отдельной внутренней дугой -6..6 dB. Она не двигает Level.

GainTelemetryClient проверяет source/revision, bounded frame shape, известные
уникальные paths и finite values. Измерения хранятся отдельно от accepted state.
Compression/Limiter reduction принимаются и доступны в latest frame, без индикаторов.
Wire contract: [Managed Telemetry](../../../Consolidator.Managed/Docs/Aspects/Telemetry.md).

Presentation применяет frame только при готовом target и prepared audio. Смена
track/bank/audio configuration очищает overlay. Смена режима очищает прежнее
измерение и отменяет gesture, если его параметр стал read-only. Показания не
отправляются через write и не создают history/persistence.

Отключение Leveler отправляет только обычный flag intent. Managed фиксирует slow
gain каждого участника группы самостоятельно в одной transaction. State/edit policy:
[GainStaging](../../../Consolidator.Managed/Docs/Aspects/GainStaging.md).
