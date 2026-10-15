# OutputDevice

[Исходник](../../Audio/Devices/OutputDevice.cpp).

GainStage -> [Limiter](LimiterProcessor.md). Это последний device цепочки.
GainStage использует тот же [Leveler](LevelerProcessor.md), что и Input,
но со своими controls, detector EQ, histories и meter.

При Leveler=false применяется manual Level; при true Target управляет
автоматическим gain. Detector capture идёт до gain, main RMS meter — после gain,
до limiter. Final output meter в AudioRuntime измеряет выход после limiter.

Limiter всегда включён в активной Output секции и использует Limit как потолок.
OutputDevice владеет bypass transition. Reduction передаётся отдельно от RMS.
Полная механика: [Runtime](Runtime.md), [GainStaging](../../../Consolidator.Managed/Docs/Aspects/GainStaging.md).
