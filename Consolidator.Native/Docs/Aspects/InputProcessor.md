# InputDevice

[Исходник](../../Audio/Devices/InputDevice.cpp).

Width -> GainStage. Width: mid=(L+R)/2, side=(L-R)/2*width, выход mid±side.
Нейтральная Width=1, smoothing 10 мс, действие только в stereo.

GainStage содержит собственный DetectorEqualizer и [Leveler](LevelerProcessor.md).
Capture идёт после Width, до gain. Детектор не измеряет собственную коррекцию.
Отдельный RMS meter измеряет main после gain для ручного режима Target.

InputDevice владеет bypass transition и дочерними histories. Общая механика
GainStage/телеметрии описана в [Runtime](Runtime.md).
