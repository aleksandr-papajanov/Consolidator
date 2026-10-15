# PolishDevice

[Исходник](../../Audio/Devices/PolishDevice.cpp).

Вход -> [Thick](Thick.md) -> [Air](Air.md). Каждый добавляет enhancement к входящему
сигналу; усреднения двух dry+wet веток нет. Controls 0..1 сглаживаются внутри algorithms.
PolishDevice владеет activity/bypass WetTransition.

Immutable configuration принадлежит Native algorithm headers. Histories принадлежат
дочерним algorithms; research overrides и API изменения model constants отсутствуют.
