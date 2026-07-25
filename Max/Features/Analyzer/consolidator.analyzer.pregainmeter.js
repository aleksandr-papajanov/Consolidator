autowatch = 1;
inlets = 1;
outlets = 0;

mgraphics.init();
mgraphics.relative_coords = 0;
mgraphics.autofill = 0;

include("JS/MeterRenderer.js");

var meterView = new GainMeterView("PRE GAIN RMS", 0, 1);

function paint() {
    meterView.Draw();
}

function gain_levels() {
    meterView.SetLevels(arrayfromargs(arguments));
}

function inletassist() {
    assist("gain_levels <inputPreDb> <inputPostDb> <outputPreDb> <outputPostDb> <referenceDb>");
}

setinletassist(-1, inletassist);
