# SaturatorDevice

[Исходник](../../Audio/Devices/SaturatorDevice.cpp).

Private DetectorEqualizer -> [Saturation](SaturationProcessor.md) -> output GainRamp.
Detector не смешивается с main. Activity/bypass WetTransition принадлежит устройству.
После завершения перехода в bypass сбрасываются дочерние histories и output ramp.
Математика saturation и расположение выходного gain сохранены.
