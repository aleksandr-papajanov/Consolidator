# InputProcessor

[Исходник](../../External/Dsp/InputProcessor.cpp).

Width через mid=(L+R)/2, side=(L-R)/2*width, выход mid±side; нейтральная Width=1.

Затем detector EQ и Leveler перед Input Gain.

Width сглаживается 10 мс и действует только в stereo.

Детектор не измеряет собственную коррекцию.
