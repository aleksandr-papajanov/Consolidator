autowatch = 1;
inlets = 1;
outlets = 0;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

include("JS/MeterRenderer.js");

var levelDeltaDb = 0;
var meterView = new AnalogMeterView("SATURATION", 40, [0, 10, 20, 30, 40], function(value) {
    return value.toFixed(1) + "%  " + (levelDeltaDb >= 0 ? "+" : "") + levelDeltaDb.toFixed(1) + " dB";
});

function paint() {
    meterView.Draw();
}

function processor_telemetry() {
    var nonlinearRatio = Number(arguments[1]);
    var nextLevelDeltaDb = Number(arguments[2]);
    if (!isFinite(nonlinearRatio) || !isFinite(nextLevelDeltaDb)) return;
    levelDeltaDb = nextLevelDeltaDb;
    meterView.SetValue(Math.max(0, nonlinearRatio) * 100);
}

function status() {}

function inletassist() {
    assist("processor_telemetry <compressorReductionDb> <saturationNonlinearRatio> <saturationLevelDeltaDb> <inputPreDb> <inputPostDb> <outputPreDb> <outputPostDb> <compressorOutputDb> <saturatorOutputDb>");
}

setinletassist(-1, inletassist);
