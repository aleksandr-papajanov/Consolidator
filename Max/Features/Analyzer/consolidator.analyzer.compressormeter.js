autowatch = 1;
inlets = 1;
outlets = 0;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

include("JS/MeterRenderer.js");

var meterView = new AnalogMeterView("COMP REDUCTION", 24, [0, 3, 6, 9, 12, 15, 18, 21, 24], function(value) {
    return "-" + value.toFixed(1) + " dB";
}, [0, 12, 24]);

function paint() {
    meterView.Draw();
}

function processor_telemetry() {
    meterView.SetValue(Math.max(0, -Number(arguments[0]) || 0));
}

function status() {}

function inletassist() {
    assist("processor_telemetry <compressorReductionDb> <saturationNonlinearRatio> <saturationLevelDeltaDb> <inputPreDb> <inputPostDb> <outputPreDb> <outputPostDb> <compressorOutputDb> <saturatorOutputDb>");
}

setinletassist(-1, inletassist);
